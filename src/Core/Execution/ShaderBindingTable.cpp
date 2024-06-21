#include <Core/Execution/ShaderBindingTable.hpp>

namespace vkl
{
	VkStridedDeviceAddressRegionKHR ShaderBindingTable::convert(Segment const& s) const
	{
		const VkDeviceAddress base_address = _buffer.buffer()->instance()->deviceAddress();
		const uint32_t shader_record_size = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleSize;
		const uint32_t shader_record_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleAlignment;
		const uint32_t shader_group_base_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupBaseAlignment;
		const uint32_t stride = std::alignUpAssumePo2<uint32_t>(shader_record_size + s.shader_data_record_size, shader_record_align);
		VkStridedDeviceAddressRegionKHR res{
			.deviceAddress = base_address + s.base,
			.stride = stride,
			.size = stride * s.count,
		};
		assert((res.deviceAddress % shader_group_base_align) == 0);
		return res;
	}

	VkStridedDeviceAddressRegionKHR ShaderBindingTable::getRaygenRegion()const
	{
		return convert(_segments[static_cast<uint32_t>(ShaderRecordType::RayGen)]);
	}

	VkStridedDeviceAddressRegionKHR ShaderBindingTable::getMissRegion()const
	{
		return convert(_segments[static_cast<uint32_t>(ShaderRecordType::Miss)]);
	}

	VkStridedDeviceAddressRegionKHR ShaderBindingTable::getHitGroupRegion()const
	{
		return convert(_segments[static_cast<uint32_t>(ShaderRecordType::HitGroup)]);
	}

	VkStridedDeviceAddressRegionKHR ShaderBindingTable::getCallableRegion()const
	{
		return convert(_segments[static_cast<uint32_t>(ShaderRecordType::Callable)]);
	}

	VkStridedDeviceAddressRegionKHR ShaderBindingTable::getRegion(ShaderRecordType record_type) const
	{
		const uint32_t index = static_cast<uint32_t>(record_type);
		assert(index < _segments.size());
		return convert(_segments[index]);
	}

	ShaderBindingTable::Regions ShaderBindingTable::getRegions() const
	{
		return Regions{
			.raygen = getRaygenRegion(),
			.miss = getMissRegion(),
			.hit_group = getHitGroupRegion(),
			.callable = getCallableRegion(),
		};
	}


	ShaderBindingTable::ShaderBindingTable(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_pipeline(ci.pipeline),
		_buffer(HostManagedBuffer::CI{
			.app = ci.app,
			.name = name() + ".Buffer",
			.size = 1024,
			.min_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupBaseAlignment,
			.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | ci.extra_buffer_usage,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		})
	{
		const uint32_t shader_record_size = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleSize;
		const uint32_t shader_record_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleAlignment;
		const uint32_t shader_group_base_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupBaseAlignment;

		const Capacity * capacities = &ci.raygen;
		uint32_t total_shader_records = 0;
		uint32_t total_shader_data = 0;
		uint32_t segment_base = 0;
		for (size_t i = 0; i < _segments.size(); ++i)
		{
			_segments[i] = Segment{
				.base = 0,
				.shader_data_record_size = capacities[i].data_size,
				.count = capacities[i].count,
				.indices_invalidation = Range{
					.begin = 0,
					.len = Range::Index(-1),
				},
				.data_invalidation = Range{
					.begin = 0,
					.len = Range::Index(-1),
				},
				.shader_index_base = total_shader_records,
				.shader_data_base = total_shader_data,
			};
			total_shader_records += _segments[i].count;
			total_shader_data += _segments[i].count * _segments[i].shader_data_record_size;
		}
		_shader_record_indices.resize(total_shader_records);

		_pipeline->setInvalidationCallback(Callback{
			.callback = [this]()
			{
				signalPipelineInvalidation();
			},
			.id = this,
		});
	}

	void ShaderBindingTable::signalPipelineInvalidation()
	{
		for (size_t i = 0; i < _segments.size(); ++i)
		{
			_segments[i].indices_invalidation = Range{.begin = 0, .len = Range::Index(-1)};
		}
	}

	uint32_t ShaderBindingTable::prepareSize()
	{
		const uint32_t shader_record_size = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleSize;
		const uint32_t shader_record_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleAlignment;
		const uint32_t shader_group_base_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupBaseAlignment;
		const uint32_t max_shader_group_size = application()->deviceProperties().ray_tracing_pipeline_khr.maxShaderGroupStride;

		assert((shader_group_base_align % shader_record_align) == 0);

		uint32_t res = 0;
		
		for (size_t i = 0; i < _segments.size(); ++i)
		{
			Segment & s = _segments[i];
			s.base = res;

			const uint32_t sbt_record_size = std::alignUpAssumePo2(shader_record_size, shader_record_align);

			// shader_group_base_align is not guaranteed to be a power of 2 (although it almost certainly is)
			res += std::alignUp(sbt_record_size * s.count, shader_group_base_align);
		}

		return res;
	}

	void ShaderBindingTable::writeData()
	{
		const uint32_t shader_record_size = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleSize;
		const uint32_t shader_record_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupHandleAlignment;
		const uint32_t shader_group_base_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupBaseAlignment;
		RayTracingPipelineInstance * pipeline = static_cast<RayTracingPipelineInstance*>(_pipeline->instance().get());
		assert(pipeline);
		RayTracingProgramInstance * program = pipeline->program();
		
		const uint8_t* handles = pipeline->shaderGroupHandles().data();
		
		for (size_t i = 0; i < _segments.size(); ++i)
		{
			Segment & s = _segments[i];
			const uint32_t record_size = shader_record_size + s.shader_data_record_size;
			const uint32_t record_stride = std::alignUpAssumePo2(record_size, shader_record_align);
			Range range = s.indices_invalidation;
			range.len = std::min(s.count, range.len);
			for (uint32_t j = 0; j < range.len; ++j)
			{
				const uint32_t group_id = j + range.begin;
				const uint32_t handle_id_per_group = _shader_record_indices[s.shader_index_base + group_id];
				const uint32_t handle_id = program->getGroupBeginIndex(i) + handle_id_per_group;
				
				const uint32_t record_offset = s.base + group_id * record_stride;
				const uint8_t* handle_ptr = handles + handle_id * shader_record_size;
				_buffer.setIFN(record_offset, handle_ptr, shader_record_size);
			}
			s.indices_invalidation = Range{.begin = 0, .len = 0};
		}
	}

	ShaderBindingTable::~ShaderBindingTable()
	{
		_pipeline->removeInvalidationCallback(this);
	}

	void ShaderBindingTable::updateResources(UpdateContext& ctx)
	{
		if (ctx.updateTick() <= _latest_update_tick)
		{
			return;
		}
		_latest_update_tick = ctx.updateTick();

		const uint32_t shader_group_base_align = application()->deviceProperties().ray_tracing_pipeline_khr.shaderGroupBaseAlignment;
		if (_update_task)
		{
			assert(false);
			_update_task->waitIFN();
			_update_task = nullptr;
		}

		_pipeline->updateResources(ctx);

		const uint32_t sbt_buffer_size = prepareSize();
		_buffer.resizeIFN(sbt_buffer_size);

		auto update_f = [this](UpdateContext* ctx)
		{
			if (ctx)
			{
				_buffer.buffer()->mutex().lock();
			}

			writeData();

			if (ctx)
			{
				_buffer.buffer()->mutex().unlock();
			}
		};
		
		const bool emit_delayed_task = _pipeline->instanceIsPending();
		
		if (emit_delayed_task)
		{
			_update_task = std::make_shared<AsynchTask>(AsynchTask::CI{
				.name = name() + ".Update",
				.verbosity = 0,
				.priority = TaskPriority::ASAP(),
				.lambda = [this, update_f]()
				{
					update_f(nullptr);
					return AsynchTask::ReturnType{
						.success = true,
					};
				},
				.dependencies = {_pipeline->getInstanceCreationTask()}
			});
			application()->threadPool().pushTask(_update_task);
		}
		else
		{
			update_f(&ctx);
		}

		if (emit_delayed_task)
		{
			_buffer.buffer()->mutex().lock();
		}

		_buffer.updateResources(ctx);

		if (emit_delayed_task)
		{
			_buffer.buffer()->mutex().unlock();
		}
	}

	void ShaderBindingTable::recordUpdateIFN(ExecutionRecorder& exec)
	{
		if (_update_task)
		{
			_update_task->waitIFN();
			_update_task = nullptr;
		}
		_buffer.recordTransferIFN(exec);
	}

	void ShaderBindingTable::setRecordProperties(ShaderRecordType record_type, Capacity capacity)
	{
		NOT_YET_IMPLEMENTED;
		const uint32_t segment_index = static_cast<uint32_t>(record_type);
		assert(segment_index < _segments.size());
		Segment s = _segments[segment_index];
		const Capacity old_capacity = Capacity{
			.count = s.count,
			.data_size = s.shader_data_record_size,
		};
		bool changed_data_size = false;
		bool changed_count = false;
		if (capacity.data_size != uint32_t(-1) && old_capacity.data_size != capacity.data_size)
		{
			s.shader_data_record_size = capacity.data_size;	
			changed_data_size = true;

			NOT_YET_IMPLEMENTED;
			for (size_t i = segment_index; i < _segments.size(); ++i)
			{
				
			}
		}
		if (capacity.count != uint32_t(-1) && old_capacity.count != capacity.count)
		{
			s.count = capacity.count;
			changed_count = true;
			const size_t new_indices_size = _shader_record_indices.size() - old_capacity.count + s.count;
			_shader_record_indices.resize(new_indices_size);
			if (capacity.count > old_capacity.count)
			{
				uint32_t shader_index_base = s.shader_index_base + s.count;
				for (size_t i = segment_index + 1; i < _segments.size(); ++i)
				{
					uint32_t * dst = _shader_record_indices.data() + shader_index_base;
					const uint32_t * src = _shader_record_indices.data() + _segments[i].shader_data_base;
					std::copy_n(src, _segments[i].count, dst);
				
					_segments[i].shader_index_base = shader_index_base;
					shader_index_base += _segments[i].count;
				}
			}
			else
			{
				for (size_t i = _segments.size() - 1; i > segment_index; --i)
				{

				}
			}
		}
	}

	void ShaderBindingTable::setRecord(ShaderRecordType record_type, uint32_t index, uint32_t shader_group_index, uint32_t data_size, const void* data)
	{
		const uint32_t segment_index = static_cast<uint32_t>(record_type);
		assert(segment_index < _segments.size());
		Segment s = _segments[segment_index];

#if VKL_SBT_USE_HOST_STORAGE == VKL_SBT_HOST_STORAGE_UNIFIED
		uint32_t & dst_shader_group_index = _shader_record_indices[s.shader_index_base + index];
		uint8_t * dst_shader_data = _shader_record_data.data() + s.shader_data_base;
#else

#endif
		if(shader_group_index != uint32_t(-1) && shader_group_index != dst_shader_group_index)
		{	
			dst_shader_group_index = shader_group_index;
			s.indices_invalidation |= index;
		}
		if (data && data_size > 0)
		{
			assert(data_size <= s.shader_data_record_size);
			std::memcpy(dst_shader_data, data, std::max(data_size, s.shader_data_record_size));
			s.data_invalidation |= index;
		}
	}
}