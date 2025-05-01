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
			},
			.default_index = 0,
		});
	}


	void LightTransport::updateResources(UpdateContext& ctx)
	{
		_light_tracer_buffer->updateResource(ctx);
		ctx.resourcesToUpdateLater() += _path_tracer_rq;
		ctx.resourcesToUpdateLater() += _light_tracer_rq;
		ctx.resourcesToUpdateLater() += _resolve_light_tracer;
		VkExtent3D extent = _target->image()->extent().value();
		_light_tracer_samples = extent.width * extent.height;
	}

	void LightTransport::render(ExecutionRecorder& exec)
	{
		if (_method == Method::PathTracer)
		{
			exec(_path_tracer_rq);
		}
		else if (_method == Method::LightTracer)
		{
			exec(application()->getPrebuiltTransferCommands().fill_buffer.with(FillBuffer::FillInfo{
				.buffer = _light_tracer_buffer,
			}));

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
				.udims_minus_1 = Vector2u(extent.width - 1, extent.height - 1),
				.dispatched_threads = float(_light_tracer_samples),
			};

			exec(_light_tracer_rq->with(ComputeCommand::SingleDispatchInfo{
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));
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
		}
	}
}