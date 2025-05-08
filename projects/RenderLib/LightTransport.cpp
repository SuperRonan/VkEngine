#include "LightTransport.hpp"

#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

namespace vkl
{
	LightTransport::LightTransport(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_scene(ci.scene),
		_target(ci.target),
		_ubo(ci.ubo),
		_sets_layouts(ci.sets_layout)
	{
		createInternals();
	}


	void LightTransport::createInternals()
	{
		const std::filesystem::path shaders = "RenderLibShaders:/RT";
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
				res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
			},
		});

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
			.hold_instance = [this](){return useLightTracer();},
		});

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
				res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
			},
		});

		_bdpt_scratch_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".BDPTScratchBuffer",
			.size = [this]() -> size_t {
				VkExtent3D extent = _target->image()->extent().value();
				extent.width = std::alignUpAssumePo2<u32>(extent.width, 32);
				extent.height = std::alignUpAssumePo2<u32>(extent.height, 32);
				size_t pixels = extent.width * extent.height * extent.depth;
				const size_t vertex_size = sizeof(vec4) * 8; // Conservativly high size TODO use the correct size
				size_t res = vertex_size * 2 * (_max_depth + 2) * pixels;
				return res;
			},
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.hold_instance = [this]() { return _method == Method::BidirectionalPathTracer; },
		});

		_bdpt_rq = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".BDPT",
			.shader_path = shaders / "BDPT.slang",
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
				Binding{
					.buffer = _bdpt_scratch_buffer,
					.binding = 4,
				},
			},
			.definitions = [this](DefinitionsList& res) {
				res.clear();
				res.pushBackFormatted("MAX_DEPTH {}", _max_depth);
			},
		});

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
					.desc = "BiDirectional Path Tracer",
				},
			},
			.default_index = 0,
		});
	}

	bool LightTransport::useLightTracer() const
	{
		return _method == Method::LightTracer || _method == Method::BidirectionalPathTracer;
	}


	void LightTransport::updateResources(UpdateContext& ctx)
	{
		_light_tracer_buffer->updateResource(ctx);
		_bdpt_scratch_buffer->updateResource(ctx);
		if (_method == Method::PathTracer)
		{
			ctx.resourcesToUpdateLater() += _path_tracer_rq;
		}
		else if (useLightTracer())
		{
			ctx.resourcesToUpdateLater() += _resolve_light_tracer;
			if (_method == Method::LightTracer)
			{
				ctx.resourcesToUpdateLater() += _light_tracer_rq;
			}
			else if (_method == Method::BidirectionalPathTracer)
			{
				ctx.resourcesToUpdateLater() += _bdpt_rq;
			}
		}
		VkExtent3D extent = _target->image()->extent().value();
		_light_tracer_samples = size_t(extent.width * extent.height * _light_tracer_sample_mult);
	}

	void LightTransport::render(ExecutionRecorder& exec)
	{
		if (_method == Method::PathTracer)
		{
			exec(_path_tracer_rq);
		}
		else if (useLightTracer())
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
				};
				VkExtent3D extent = _target->image()->extent().value();
				vec2 dims = vec2(extent.width, extent.height);
				LightTracerPC pc{
					.oo_dims = rcp(dims),
					.dims = dims,
					.udims_minus_1 = Vector2u(extent.width, extent.height),
					.dispatched_threads = float(_light_tracer_samples),
				};

				exec(_light_tracer_rq->with(ComputeCommand::SingleDispatchInfo{
					.pc_data = &pc,
					.pc_size = sizeof(pc),
				}));
			}
			else if (_method == Method::BidirectionalPathTracer)
			{
				exec(_bdpt_rq);
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

			ImGui::InputInt("Max Depth", (int*)&_max_depth);
			_max_depth = std::clamp<uint>(_max_depth, 1, 16);
			
			if (_method == Method::LightTracer)
			{
				ImGui::SliderFloat("Samples multiplicator", &_light_tracer_sample_mult, 1, 16, "%.3f", ImGuiSliderFlags_Logarithmic);
				const u32 local_size = 256;
				u32 samples = std::alignUpAssumePo2(_light_tracer_samples, local_size);
				ImGui::Text("Light Tracer Samples: %d", samples);
			}
		}
	}
}