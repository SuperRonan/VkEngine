#include "LightTransport.hpp"

#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include <ShaderLib/Spectrum/SpectrumDefinitions.h>

#include <vkl/IO/ImGuiUtils.hpp>

namespace vkl
{
	LightTransport::LightTransport(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_scene(ci.scene),
		_target(ci.target),
		_ubo(ci.ubo),
		_sets_layouts(ci.sets_layout)
	{
		_spectrum_mode = SPECTRUM_MODE_RGB;

		_max_scratch_buffer_size = application()->deviceProperties().props_11.maxMemoryAllocationSize;

		createInternals();
	}


	void LightTransport::createInternals()
	{
		const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;
		const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;

		if (_use_rt_pipelines && !can_rt)
		{
			_use_rt_pipelines = false;
		}
		else if (!_use_rt_pipelines && !can_rq)
		{
			_use_rt_pipelines = true;
		}

		const std::filesystem::path shaders = "RenderLibShaders:/RenderLib/RT";

		using RTShader = RayTracingCommand::RTShader;

		MyVector<RTShader> miss_shaders = {
			RTShader{.path = shaders / "TraceRT.slang"},
			RTShader{.path = shaders / "VisibilityRT.slang"},
		};

		MyVector<RTShader> chit_shaders = {
			RTShader{.path = shaders / "TraceRT.slang"},
			RTShader{.path = shaders / "VisibilityRT.slang"},
		};

		MyVector<RTShader> ahit_shaders = {
			RTShader{.path = shaders / "TraceRT.slang"},
			RTShader{.path = shaders / "VisibilityRT.slang"},
		};

		MyVector<RayTracingCommand::HitGroup> hit_groups = {
			RayTracingCommand::HitGroup{
				.closest_hit = 0,
				.any_hit = 0,
			},
			RayTracingCommand::HitGroup{
				.closest_hit = 1,
				.any_hit = 1,
			},
		};

		auto fill_sbt = [&](RayTracingCommand& command)
		{
			ShaderBindingTable* sbt = command.getSBT().get();
			sbt->setRecord(ShaderRecordType::RayGen, 0, 0);
			sbt->setRecord(ShaderRecordType::Miss, 0, 0);
			sbt->setRecord(ShaderRecordType::Miss, 1, 1);
			sbt->setRecord(ShaderRecordType::HitGroup, 0, 0);
			sbt->setRecord(ShaderRecordType::HitGroup, 1, 1);
		};
		 
		if (can_rq)
		{
			_path_tracer_rq = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = application(),
				.name = name() + ".PathTracer",
				.shader_path = shaders / "PathTracing.slang",
				.extent = _target->image()->extent(),
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 1,
					},
					Binding{
						.image = _target,
						.binding = 2,
					},
				},
				.definitions = [this](DefinitionsList& res) {
					res.clear();
					res.pushBack(_target_format_str);
					res.pushBack(_spectrum_mode_str);
					if (_compile_time_max_depth)
						res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
					if(_Li_resampling != 0)
						res.pushBack("USE_Li_RESAMPLING");
				},
			});
		}
		

		
		if (can_rt)
		{
			_path_tracer_rt = std::make_shared<RayTracingCommand>(RayTracingCommand::CI{
				.app = application(),
				.name = name() + ".PathTracer",
				.sets_layouts = _sets_layouts,
				.raygen = RTShader{.path = shaders / "PathTracing.slang"},
				.misses = miss_shaders,
				.closest_hits = chit_shaders,
				.any_hits = ahit_shaders,
				.hit_groups = hit_groups,
				.definitions = [this](DefinitionsList& res) {
					res.clear();
					res.pushBack(_target_format_str);
					res.pushBack(_spectrum_mode_str);
					if(_compile_time_max_depth)
						res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
					if (_Li_resampling != 0)
						res.pushBack("USE_Li_RESAMPLING");
					
				},
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 1,
					},
					Binding{
						.image = _target,
						.binding = 2,
					},
				},
				.extent = _target->image()->extent(),
				.max_recursion_depth = 0,
				.create_sbt = true,
			});

			fill_sbt(*_path_tracer_rt);
		}

		_light_tracer_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".LightTracerBuffer",
			.size = [this]() -> size_t {
				VkExtent3D extent = _target->image()->extent().value();
				size_t pixels = extent.width * extent.height * extent.depth;
				constexpr const size_t pixel_size = sizeof(vec4);
				return pixels * pixel_size;
			},
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.hold_instance = [this](){return usingLightTracer();},
		});

		if (can_rq)
		{
			_light_tracer_rq = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = application(),
				.name = name() + ".LightTracer",
				.shader_path = shaders / "LightTracing.slang",
				.extent = [this](){return VkExtent3D{.width = _light_tracer_samples, .height = 1, .depth = 1}; },
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 1,
					},
					Binding{
						.buffer = _light_tracer_buffer,
						.binding = 2,
					},
				},
				.definitions = [this](DefinitionsList& res) {
					res.clear();
					res.pushBack(_spectrum_mode_str);
					if (_compile_time_max_depth)
						res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
				},
			});
		}
		
		if (can_rt)
		{
			_light_tracer_rt = std::make_shared<RayTracingCommand>(RayTracingCommand::CI{
				.app = application(),
				.name = name() + ".LightTracer",
				.sets_layouts = _sets_layouts,
				.raygen = RTShader{.path = shaders / "LightTracing.slang"},
				.misses = miss_shaders,
				.closest_hits = chit_shaders,
				.any_hits = ahit_shaders,
				.hit_groups = hit_groups,
				.definitions = [this](DefinitionsList& res) {
					res.clear();
					res.pushBack(_spectrum_mode_str);
					if (_compile_time_max_depth)
						res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
				},
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 1,
					},
					Binding{
						.buffer = _light_tracer_buffer,
						.binding = 2,
					},
				},
				.extent = [this]() {return VkExtent3D{.width = _light_tracer_samples, .height = 1, .depth = 1}; },
				.max_recursion_depth = 0,
				.create_sbt = true,
			});
			fill_sbt(*_light_tracer_rt);
		}

		_bdpt_scratch_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".BDPTScratchBuffer",
			.size = &_bdpt_scratch_buffer_size,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.hold_instance = [this]() { return _method == Method::BidirectionalPathTracer; },
		});

		auto bdpt_dispatch_size = [this]() -> VkExtent3D
		{
			VkExtent3D res = *_target->image()->extent();
			res.height = _bdpt_dispatch_height;
			return res;
		};

		if (can_rq)
		{
			_bdpt_rq = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = application(),
				.name = name() + ".BDPT",
				.shader_path = shaders / "BDPT.slang",
				.extent = bdpt_dispatch_size,
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 1,
					},
					Binding{
						.image = _target,
						.binding = 2,
					},
					Binding{
						.buffer = _light_tracer_buffer,
						.binding = 3,
					},
					Binding{
						.buffer = {_bdpt_scratch_buffer, [this]() {return Buffer::Range{
							.begin = 0,
							.len = _bdpt_scratch_buffer_segment_2,
						}; }},
						.binding = 4,
					},
					Binding{
						.buffer = {_bdpt_scratch_buffer, [this](){return Buffer::Range{
							.begin = _bdpt_scratch_buffer_segment_2,
							.len = VK_WHOLE_SIZE,
						}; }},
						.binding = 5,
					},
				},
				.definitions = [this](DefinitionsList& res) {
					res.clear();
					res.pushBack(_target_format_str);
					res.pushBack(_spectrum_mode_str);
					if (_compile_time_max_depth)
						res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
				},
			});
		}

		if (can_rt)
		{
			_bdpt_rt = std::make_shared<RayTracingCommand>(RayTracingCommand::CI{
				.app = application(),
				.name = name() + ".BDPT",
				.sets_layouts = _sets_layouts,
				.raygen = RTShader{.path = shaders / "BDPT.slang"},
				.misses = miss_shaders,
				.closest_hits = chit_shaders,
				.any_hits = ahit_shaders,
				.hit_groups = hit_groups,
				.definitions = [this](DefinitionsList& res) {
					res.clear();
					res.pushBack(_target_format_str);
					res.pushBack(_spectrum_mode_str);
					if (_compile_time_max_depth)
						res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
				},
				.bindings = {
					Binding{
						.buffer = _ubo,
						.binding = 1,
					},
					Binding{
						.image = _target,
						.binding = 2,
					},
					Binding{
						.buffer = _light_tracer_buffer,
						.binding = 3,
					},
					Binding{
						.buffer = {_bdpt_scratch_buffer, [this]() {return Buffer::Range{
							.begin = 0,
							.len = _bdpt_scratch_buffer_segment_2,
						}; }},
						.binding = 4,
					},
					Binding{
						.buffer = {_bdpt_scratch_buffer, [this]() {return Buffer::Range{
							.begin = _bdpt_scratch_buffer_segment_2,
							.len = VK_WHOLE_SIZE,
						}; }},
						.binding = 5,
					},
				},
				.extent = bdpt_dispatch_size,
				.max_recursion_depth = 0,
				.create_sbt = true,
			});
			fill_sbt(*_light_tracer_rt);
		}


		_resolve_light_tracer = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".ResolveLightTracer",
			.shader_path = shaders / "ResolveLightTracing.slang",
			.extent = _target->image()->extent(),
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.buffer = _ubo,
					.binding = 1,
				},
				Binding{
					.image = _target,
					.binding = 2,
				},
				Binding{
					.buffer = _light_tracer_buffer,
					.binding = 3,
				},
			},
			.definitions = [this](DefinitionsList& res) {
				res.clear();
				res.pushBack(_target_format_str);
				std::string_view resolve_mode;
				if (_method == Method::LightTracer)
				{
					resolve_mode = "RESOLVE_MODE_OVERWRITE";
				}
				else if (_method == Method::BidirectionalPathTracer)
				{
					resolve_mode = "RESOLVE_MODE_ADD";
				}
				if (!resolve_mode.empty())
				{
					res.pushBackFormatted("RESOLVE_MODE {}", resolve_mode);
				}
			},
		});

		_method_selection = ImGuiListSelection(ImGuiListSelection::CI{
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.same_line = true,
			.options = {
				ImGuiListSelection::Option{
					.name = "Path Tracing",
				},
				ImGuiListSelection::Option{
					.name = "Light Tracing",
				},
				ImGuiListSelection::Option{
					.name = "BDPT",
					.desc = 
						"BiDirectional Path Tracer\n"
						"Requires a large scratch buffer!\n"
						"Visibility rays are currently disabled with RT pipelines."
					,
				},
			},
			.default_index = 0,
		});
	}

	bool LightTransport::usingLightTracer() const
	{
		return _method == Method::LightTracer || _method == Method::BidirectionalPathTracer;
	}


	void LightTransport::updateResources(UpdateContext& ctx)
	{
		if (_spectrum_mode & _SPECTRUM_FLAG_SAMPLED)
		{
			// TODO Check that Sampled mode is limited up to 4 samples
			
		}
		else
		{
			// Enforce that RGB and XYZ has 3 samples
			_spectrum_mode = (_spectrum_mode & _SPECTRUM_FLAG_MASK) | 3;
		}
		if (_spectrum_mode_str.empty())
		{
			_spectrum_mode_str = std::format("LIGHT_TRANSPORT_SPECTUM_MODE {0:#0x}", _spectrum_mode);
		}

		_light_tracer_buffer->updateResource(ctx);
		
		// Recompute BDPT scratch buffer memory requirement and dispatch size
		{
			VkExtent3D extent = _target->image()->extent().value();
			extent.width = std::alignUpAssumePo2<u32>(extent.width, 32);
			extent.height = std::alignUpAssumePo2<u32>(extent.height, 32);
			size_t pixels = extent.width * extent.height * extent.depth;
			// 5 vec4 at most for now (with uncompressed)
			// 4 vec4 with minimum compression (fastest)
			// 3 vec4 with aggressive compression, (but slower)
			const uint max_vertices = _max_depth + 2;
			const size_t vertex_size = sizeof(vec4) * 3;
			const size_t pdf_pair_size = sizeof(vec2);
			_bdpt_needed_scratch_size = pixels * max_vertices * 2 * (vertex_size + pdf_pair_size);
			size_t needed_vertex_size = pixels * max_vertices * 2 * (vertex_size);

			const size_t max_buffer_size = application()->deviceProperties().props_11.maxMemoryAllocationSize;
			const size_t max_range_size = size_t(application()->deviceProperties().props2.properties.limits.maxStorageBufferRange);

			size_t max_scratch_buffer_size = std::min(_max_scratch_buffer_size, max_buffer_size);

			size_t alloc_divisions = std::divUpAssumeNoOverflow(_bdpt_needed_scratch_size, max_scratch_buffer_size);
			size_t range_division = std::divUpAssumeNoOverflow(needed_vertex_size, max_range_size);
			_bdpt_divisions = std::max(alloc_divisions, range_division);

			_bdpt_dispatch_height = extent.height / _bdpt_divisions;

			size_t reduced_pixels = extent.width * _bdpt_dispatch_height * extent.depth;

			_bdpt_scratch_buffer_size = reduced_pixels * max_vertices * 2 * (vertex_size + pdf_pair_size);
			_bdpt_scratch_buffer_segment_2 = reduced_pixels * max_vertices * 2 * vertex_size;
		}
		_bdpt_scratch_buffer->updateResource(ctx);
		VkFormat target_format = _target->format().value();
		if (target_format != _target_format)
		{
			_target_format = target_format;
			_target_format_str = std::format("TARGET_IMAGE_FORMAT {}", DetailedVkFormat::Find(_target_format).getGLSLName());
		}
		if (_method == Method::PathTracer)
		{
			if (_use_rt_pipelines)
			{
				ctx.resourcesToUpdateLater() += _path_tracer_rt;
			}
			else
			{
				ctx.resourcesToUpdateLater() += _path_tracer_rq;
			}
		}
		else if (usingLightTracer())
		{
			ctx.resourcesToUpdateLater() += _resolve_light_tracer;
			if (_method == Method::LightTracer)
			{
				if (_use_rt_pipelines)
				{
					ctx.resourcesToUpdateLater() += _light_tracer_rt;
				}
				else
				{
					ctx.resourcesToUpdateLater() += _light_tracer_rq;
				}
				
			}
			else if (_method == Method::BidirectionalPathTracer)
			{
				if (_use_rt_pipelines)
				{
					ctx.resourcesToUpdateLater() += _bdpt_rt;
				}
				else
				{
					ctx.resourcesToUpdateLater() += _bdpt_rq;
				}
			}
		}
		VkExtent3D extent = _target->image()->extent().value();
		_light_tracer_samples = size_t(extent.width * extent.height * _light_tracer_sample_mult);
	}

	void LightTransport::render(ExecutionRecorder& exec)
	{
		if (_method == Method::PathTracer)
		{
			if (_use_rt_pipelines)
			{
				_path_tracer_rt->getSBT()->recordUpdateIFN(exec);
				exec(_path_tracer_rt);
			}
			else
			{
				exec(_path_tracer_rq);
			}
		}
		else if (usingLightTracer())
		{
			exec(application()->getPrebuiltTransferCommands().fill_buffer.with(FillBuffer::FillInfo{
				.buffer = _light_tracer_buffer,
			}));

			if (_method == Method::LightTracer)
			{
				struct  LightTracerPC
				{
					vec2 oo_dims;
					vec2 dims;
					Vector2u udims_minus_1;
					float dispatched_threads;
					uint32_t flags = 0;
				};
				VkExtent3D extent = _target->image()->extent().value();
				vec2 dims = vec2(extent.width, extent.height);
				LightTracerPC pc{
					.oo_dims = rcp(dims),
					.dims = dims,
					.udims_minus_1 = Vector2u(extent.width, extent.height),
					.dispatched_threads = float(_light_tracer_samples),
				};
				if (_enable_delta_connections)
				{
					pc.flags |= 0x1;
				}

				if (_use_rt_pipelines)
				{
					_light_tracer_rt->getSBT()->recordUpdateIFN(exec);
					exec(_light_tracer_rt->with(RayTracingCommand::SingleTraceInfo{
						.pc_data = &pc,
						.pc_size = sizeof(pc),
					}));
				}
				else
				{
					exec(_light_tracer_rq->with(ComputeCommand::SingleDispatchInfo{
						.pc_data = &pc,
						.pc_size = sizeof(pc),
					}));
				}
			}
			else if (_method == Method::BidirectionalPathTracer)
			{
				if (_use_rt_pipelines)
				{
					_bdpt_rt->getSBT()->recordUpdateIFN(exec);
				}
				
				struct BDPT_PC
				{
					uint32_t offset_x, offset_y;
					uint32_t flags = 0;
				};
				BDPT_PC pc{
					.offset_x = 0,
				};
				if (_enable_delta_connections)
				{
					pc.flags |= 0x1;
				}
				for (uint i = 0; i < _bdpt_divisions; ++i)
				{
					pc.offset_y = i * _bdpt_dispatch_height;
					if (_use_rt_pipelines)
					{
						exec(_bdpt_rt->with(RayTracingCommand::SingleTraceInfo{
							.pc_data = &pc,
							.pc_size = sizeof(BDPT_PC),
						}));
					}
					else
					{
						exec(_bdpt_rq->with(ComputeCommand::SingleDispatchInfo{
							.pc_data = &pc,
							.pc_size = sizeof(BDPT_PC),
						}));
					}
				}
			}
			exec(_resolve_light_tracer);
		}
	}

	void LightTransport::declareGUI(GuiContext& ctx)
	{
		if (ImGui::CollapsingHeader("LightTransport"))
		{
			if (_method_selection.declare())
			{
				_method = static_cast<Method>(_method_selection.index());
			}

			
			const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;
			const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;
			const bool can_change_rt_backend = can_rt && can_rq;
			ImGui::BeginDisabled(!can_change_rt_backend);
			ImGui::Checkbox("Use RT Pipelines", &_use_rt_pipelines);
			ImGui::SetItemTooltip("Check to use Ray Tracing Pipelines, Uncheck to use Ray Queries and compute shaders.");
			ImGui::EndDisabled();

			static thread_local ImGuiListSelection spectum_mode_selection = ImGuiListSelection::CreateInfo{
				.name = "Spectrum rendering mode",
				.mode = ImGuiListSelection::Mode::RadioButtons,
				.same_line = true,
				.options = {
					ImGuiListSelection::Option{
						.name = "RGB",
					},
					ImGuiListSelection::Option{
						.name = "XYZ",
					},
					ImGuiListSelection::Option{
						.name = "Spectal",
					},
				},
			};

			size_t index = static_cast<size_t>((_spectrum_mode >> _SPECTRUM_SAMPLES_BIT_COUNT));
			if (spectum_mode_selection.declare(index))
			{
				_spectrum_mode &= ~(std::bitMask<int>(3) << _SPECTRUM_SAMPLES_BIT_COUNT);
				_spectrum_mode |= (static_cast<int>(index) << _SPECTRUM_SAMPLES_BIT_COUNT);
				_spectrum_mode_str.clear();
			}
			bool use_sampled = (_spectrum_mode & _SPECTRUM_FLAG_MASK) == _SPECTRUM_FLAG_SAMPLED;
			ImGui::BeginDisabled(!use_sampled);
			{
				int samples = _spectrum_mode & std::bitMask<int>(_SPECTRUM_SAMPLES_BIT_COUNT);
				bool changed = ImGui::InputInt("Samples", &samples, 1, 1 /*, ImGuiInputTextFlags_EnterReturnsTrue */);
				samples = std::clamp(samples, 1, 4);

				if (changed)
				{
					_spectrum_mode &= ~std::bitMask<int>(_SPECTRUM_SAMPLES_BIT_COUNT);
					_spectrum_mode |= samples;
					_spectrum_mode_str.clear();
				}
			}
			ImGui::EndDisabled();


			ImGui::InputInt("Max Depth", (int*)&_max_depth);
			_max_depth = std::clamp<uint>(_max_depth, 1, 16);
			ImGui::Checkbox("Compile time Max Depth", &_compile_time_max_depth);
			if (_method == Method::PathTracer)
			{
				int li_resamling = static_cast<int>(_Li_resampling);
				if (ImGui::InputInt("Li Resampling", &li_resamling))
				{
					li_resamling = std::max(li_resamling, 0);
					_Li_resampling = static_cast<uint>(li_resamling);
				}
			}
			if (usingLightTracer())
			{
				ImGui::Checkbox("Enable delta light connections with a punctual camera", &_enable_delta_connections);
				ImGui::SetItemTooltip("May induce a significant performance cost for nearly no benefit");
			}
			if (_method == Method::LightTracer)
			{
				ImGui::SliderFloat("Samples multiplicator", &_light_tracer_sample_mult, 1, 16, "%.3f", ImGuiSliderFlags_Logarithmic);
				const u32 local_size = 256;
				u32 samples = std::alignUpAssumePo2(_light_tracer_samples, local_size);
				ImGui::Text("Light Tracer Samples: %d", samples);
			}

			if (_method == Method::BidirectionalPathTracer)
			{
				const size_t MiB = 1024 * 1024;
				int max_scratch_size_MiB = _max_scratch_buffer_size / MiB;
				if (ImGui::InputInt("Max BDPT scratch buffer size (MiB)", &max_scratch_size_MiB, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue & 0))
				{
					if (max_scratch_size_MiB > 128)
					{
						_max_scratch_buffer_size = max_scratch_size_MiB * MiB;
					}
				}
				size_t scratch_buffer_size_MiB = _bdpt_scratch_buffer_size / MiB;
				ImGui::Text("BDPT scratch buffer size: %llu MiB", scratch_buffer_size_MiB);
				ImGui::Text("BDPT divisions: %u", _bdpt_divisions);
			}

			ImGui::Separator();
		}
	}
}