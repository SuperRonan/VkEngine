#include <Core/Commands/AccelerationStructureCommands.hpp>

namespace vkl
{
	struct BuildAccelerationStructureCommandNode : public ExecutionNode
	{
		struct Target
		{
			std::shared_ptr<AccelerationStructureInstance> src;
			std::shared_ptr<AccelerationStructureInstance> dst;
		};
		MyVector<Target> _targets;
		MyVector<VkAccelerationStructureGeometryKHR> _geometries;
		MyVector<VkAccelerationStructureBuildGeometryInfoKHR> _build_infos;
		MyVector<VkAccelerationStructureBuildRangeInfoKHR> _build_ranges;

		std::shared_ptr<PooledBuffer> _pooled_scratch_buffer;
		BufferAndRangeInstance _scratch_buffer;
		
		MyVector<VkAccelerationStructureBuildRangeInfoKHR*> _my_ptr_build_ranges;

		struct CreateInfo
		{
			VkApplication * app;
			std::string name = {};
		};
		using CI = CreateInfo;

		BuildAccelerationStructureCommandNode(CreateInfo const& ci):
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		virtual void clear() override
		{
			_targets.clear();
			_build_infos.clear();
			_build_ranges.clear();
			_geometries.clear();
			_scratch_buffer = {};
			_pooled_scratch_buffer.reset();
			ExecutionNode::clear();
		}

		virtual void execute(ExecutionContext& ctx)
		{
			ctx.pushDebugLabel(name());

			_my_ptr_build_ranges.resize(_build_infos.size());
			size_t counter = 0;
			for (size_t i = 0; i < _build_infos.size(); ++i)
			{
				_my_ptr_build_ranges[i] = _build_ranges.data() + counter;
				counter += _build_infos[i].geometryCount;

				if (_targets[i].src && _targets[i].src != _targets[i].dst)
				{
					ctx.keepAlive(_targets[i].src);
				}
				ctx.keepAlive(_targets[i].dst);
			}

			application()->extFunctions()._vkCmdBuildAccelerationStructuresKHR(ctx.getCommandBuffer()->handle(), _build_infos.size32(), _build_infos.data(), _my_ptr_build_ranges.data());

			ctx.popDebugLabel();
			if (_pooled_scratch_buffer)
			{
				ctx.keepAlive(_pooled_scratch_buffer);
			}
		}

	};

	void BuildAccelerationStructureCommand::BuildInfo::clear()
	{
		targets.clear();
		ranges.clear();
		scratch_buffer = {};
	}

	bool BuildAccelerationStructureCommand::BuildInfo::pushIFN(std::shared_ptr<AccelerationStructure> const& as)
	{
		bool res = false;
		std::shared_ptr<AccelerationStructureInstance> asi = as->instance();

		VkBuildAccelerationStructureModeKHR build_mode = asi->buildMode();
		if (build_mode <= VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR)
		{
			targets.push_back(Target{
				.src = nullptr,
				.dst = as,
				.mode = build_mode,
			});
			if (build_mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR && targets.back().src == nullptr)
			{
				targets.back().src = as;
			}
			res = true;
		}

		asi->setBuildMode(VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR);
		return res;
	}

	std::shared_ptr<ExecutionNode> BuildAccelerationStructureCommand::getExecutionNode(RecordContext& ctx, BuildInfo const& bi)
	{
		std::shared_ptr<BuildAccelerationStructureCommandNode> node = _exec_node_cache.getCleanNode<BuildAccelerationStructureCommandNode>([&]() {
			return std::make_shared<BuildAccelerationStructureCommandNode>(BuildAccelerationStructureCommandNode::CI{
				.app = application(),
				.name = name(),
			});
		});

		node->setName(name());

		node->_targets.resize(bi.targets.size());
		node->_build_infos.resize(bi.targets.size());

		size_t scratch_buffer_size = 0;
		const size_t scratch_align = application()->deviceProperties().acceleration_structure_khr.minAccelerationStructureScratchOffsetAlignment;

		for (size_t i = 0; i < bi.targets.size(); ++i)
		{	
			node->_targets[i] = BuildAccelerationStructureCommandNode::Target{
				.src = bi.targets[i].src ? bi.targets[i].src->instance() : nullptr,
				.dst = bi.targets[i].dst->instance(),
			};

			AccelerationStructureInstance * target_as = node->_targets[i].dst.get();
			AccelerationStructureInstance * source_as = node->_targets[i].src.get();
			

			// has pointers to the AS's vector (pGeometries)
			// Copy them?
			VkAccelerationStructureBuildGeometryInfoKHR & build_info = node->_build_infos[i];
			build_info = target_as->buildInfo();
			build_info.dstAccelerationStructure = target_as->handle();
			build_info.srcAccelerationStructure = source_as ? source_as->handle() : VK_NULL_HANDLE;

			build_info.mode = bi.targets[i].mode;
			assert(build_info.mode <= VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR);
			size_t requested_scratch_size = 0;
			if (build_info.mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR)
			{
				requested_scratch_size = target_as->buildSizes().buildScratchSize;
			}
			else if (build_info.mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR)
			{
				assert(build_info.srcAccelerationStructure != VK_NULL_HANDLE);
				requested_scratch_size = target_as->buildSizes().updateScratchSize;
			}

			// Offset into future scratch buffer
			build_info.scratchData.deviceAddress = scratch_buffer_size;
			scratch_buffer_size += std::alignUp(requested_scratch_size, scratch_align);
			
			const VkAccessFlags2 extern_buffer_access = VK_ACCESS_2_SHADER_READ_BIT;
			const VkBufferUsageFlags2KHR extern_buffer_usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
			
			BLASI* blasi = dynamic_cast<BLASI*>(target_as);
			TLASI* tlasi = dynamic_cast<TLASI*>(node->_targets[i].dst.get());
			
			if (blasi)
			{
				for (size_t j = 0; j < blasi->triangleMeshGeometries().size(); ++j)
				{
					auto & triangles = blasi->triangleMeshGeometries()[j];
					node->resources() += BufferUsage{
						.bari = triangles.vertex_buffer,
						.begin_state = ResourceState2{
							.access = extern_buffer_access,
							.stage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
						},
						.usage = extern_buffer_usage,
					};
					if (triangles.index_buffer)
					{
						node->resources() += BufferUsage{
							.bari = triangles.index_buffer,
							.begin_state = ResourceState2{
								.access = extern_buffer_access,
								.stage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
							},
							.usage = extern_buffer_usage,
						};
					}
				}
			}
			else if (tlasi)
			{
				TLAS * tlas = reinterpret_cast<TLAS*>(bi.targets[i].dst.get());
				//build_info.pGeometries[0].geometry.instances.data.deviceAddress = tlasi->instancesBuffer().deviceAddress();
				node->resources() += BufferUsage{
					.bari = tlasi->instancesBuffer(),
					.begin_state = ResourceState2{
						.access = extern_buffer_access,
						.stage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
					},
					.usage = extern_buffer_usage,
				};

				for (size_t j = 0; j < tlas->blases(); ++j)
				{
					auto & blas_ref = tlas->blases()[j];
					if (blas_ref.blas)
					{
						node->resources() += BufferUsage{
							.bari = blas_ref.blas->instance()->storageBuffer(),
							.begin_state = ResourceState2{
								.access = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR, // Not 100% sure about this one, maybe shader read bit
								.stage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
							},
							.usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
						};
					}
				}
			}
			else
			{
				NOT_YET_IMPLEMENTED;
			}

			size_t previous_range_size = node->_build_ranges.size();
			node->_build_ranges.resize(previous_range_size + build_info.geometryCount);
			if (bi.targets[i].range_offset == uint32_t(-1)) // No range is provided -> use the full range of the AS
			{
				for (uint32_t j = 0; j < build_info.geometryCount; ++j)
				{
					VkAccelerationStructureBuildRangeInfoKHR & range = node->_build_ranges[previous_range_size + j];
					memset(&range, 0, sizeof(range));
					if (blasi)
					{
						range.primitiveCount = blasi->maxPrimitiveCount()[j];
					}
					else if(tlasi)
					{
						//TLAS* tlas = reinterpret_cast<TLAS*>(bi.targets[i].src.get());
						range.primitiveCount = tlasi->primitiveCount();
					}
					else
					{
						NOT_YET_IMPLEMENTED;
					}
				}
			}
			else // Use the provided range
			{
				std::copy_n(bi.ranges.begin() + bi.targets[i].range_offset, build_info.geometryCount, node->_build_ranges.begin() + previous_range_size);
			}

			node->resources() += BufferUsage{
				.bari = target_as->storageBuffer(),
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
					.stage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
				},
				.usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			};
			if (build_info.mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR)
			{
				node->resources() += BufferUsage{
					.bari = source_as->storageBuffer(),
					.begin_state = ResourceState2{
						.access = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR,
						.stage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
					},
					.usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
				};
			}
		}

		
		if (scratch_buffer_size > 0)
		{
			if (!bi.scratch_buffer || (bi.scratch_buffer.size() < scratch_buffer_size))
			{
				node->_pooled_scratch_buffer = std::make_shared<PooledBuffer>(&_scratch_buffer_pool, scratch_buffer_size);
				node->_scratch_buffer.buffer = node->_pooled_scratch_buffer->buffer();
				node->_scratch_buffer.range = Buffer::Range{
					.begin = 0,
					.len = scratch_buffer_size,
				};
			}
			else
			{
				node->_scratch_buffer = bi.scratch_buffer;
			}

			const VkDeviceAddress scratch_base_address = node->_scratch_buffer.deviceAddress();
			for (size_t i = 0; i < node->_targets.size(); ++i)
			{
				node->_build_infos[i].scratchData.deviceAddress += scratch_base_address;
			}
			node->resources() += BufferUsage{
				.bari = node->_scratch_buffer,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
					.stage = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
				},
				.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT_KHR,
			};
		}
		else
		{
			node->_scratch_buffer = {};
		}

		return node;
	}

	Executable BuildAccelerationStructureCommand::with(BuildInfo const& bi)
	{
		return [this, bi](RecordContext & ctx)
		{
			return getExecutionNode(ctx, bi);
		};
	}

	BuildAccelerationStructureCommand::BuildAccelerationStructureCommand(CreateInfo const& ci):
		DeviceCommand(ci.app, ci.name),
		_scratch_buffer_pool(BufferPool::CI{
			.app = application(),
			.name = name() + ".ScratchBufferPool",
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		})
	{}

	BuildAccelerationStructureCommand::~BuildAccelerationStructureCommand()
	{

	}
}