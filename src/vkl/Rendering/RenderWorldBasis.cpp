#include <vkl/Rendering/RenderWorldBasis.hpp>

#include <vkl/Execution/Executor.hpp>

namespace vkl
{
	RenderWorldBasis::RenderWorldBasis(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_sets_layouts(ci.sets_layouts),
		_render_pass(ci.extern_render_pass),
		_subpass_index(ci.subpass_index),
		_target(ci.target),
		_depth(ci.depth),
		_render_in_log_space(ci.render_in_log_space)
	{
		createInternals();
	}

	RenderWorldBasis::~RenderWorldBasis()
	{

	}

	void RenderWorldBasis::createInternals()
	{
		if (!_render_pass)
		{
			RenderPass::SPCI render_pass_ci{
				.app = application(),
				.name = name() + ".RenderPass",
				.colors = {
					AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Blend, _target),
				},
				.depth_stencil = AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::ReadOnly, _depth),
			};

			_render_pass = std::make_shared<RenderPass>(std::move(render_pass_ci));

			Framebuffer::CI fb_ci{
				.app = application(),
				.name = name() + ".Framebuffer",
				.render_pass = _render_pass,
				.attachments = {_target},
			};
			if (_depth)
			{
				fb_ci.attachments += _depth;
			}
			_framebuffer = std::make_shared<Framebuffer>(std::move(fb_ci));
		}

		std::filesystem::path shaders = "ShaderLib:/Rendering/Interface";

		_blending = AttachmentBlending::DefaultAlphaBlending();

		_render_planes = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".RenderPlanes",
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			.cull_mode = VK_CULL_MODE_NONE,
			.sets_layouts = _sets_layouts,
			.extern_render_pass = _render_pass,
			.subpass_index = _subpass_index,
			.color_attachments = {
				GraphicsCommand::ColorAttachment{
					.blending = &_blending,
				}
			},
			.write_depth = false,
			.depth_compare_op = VK_COMPARE_OP_LESS,
			.vertex_shader_path = shaders / "Render3DWorldPlanes.glsl",
			.geometry_shader_path = shaders / "Render3DWorldPlanes.glsl",
			.fragment_shader_path = shaders / "Render3DWorldPlanes.glsl",
			.definitions = [this](DefinitionsList& res)
			{
				res.clear();
				if (_render_in_log_space.valueOr(false))
				{
					res.pushBack("DISPLAY_IN_LOG2 1");
				}
			},
		});

		_render_lines = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".RenderLines",
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			.line_raster_state = GraphicsPipeline::LineRasterizationState{
				.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
			},
			.sets_layouts = _sets_layouts,
			.extern_render_pass = _render_pass,
			.subpass_index = _subpass_index,
			.color_attachments = {
				GraphicsCommand::ColorAttachment{
					.blending = &_blending,
				}
			},
			.write_depth = false,
			.depth_compare_op = VK_COMPARE_OP_LESS,
			.vertex_shader_path = shaders / "Render3DWorldLines.glsl",
			.geometry_shader_path = shaders / "Render3DWorldLines.glsl",
			.fragment_shader_path = shaders / "Render3DWorldLines.glsl",
			.definitions = [this](DefinitionsList& res)
			{
				res.clear();
				if (_render_in_log_space.valueOr(false))
				{
					res.pushBack("DISPLAY_IN_LOG2 1");
				}
			},
		});
	}


	void RenderWorldBasis::updateResources(UpdateContext& ctx)
	{
		if (_framebuffer)
		{
			_render_pass->updateResources(ctx);
			ctx.resourcesToUpdateLater() += _framebuffer;
		}

		ctx.resourcesToUpdateLater() += _render_planes;
		ctx.resourcesToUpdateLater() += _render_lines;
	}

	void RenderWorldBasis::execute(ExecutionRecorder& exec, Camera & camera)
	{
		if (_framebuffer)
		{
			RenderPassBeginInfo render_pass{
				.framebuffer = _framebuffer->instance(),
			};
			exec.beginRenderPass(std::move(render_pass));
		}

		const uint32_t line_count = _flags & std::bitMask<uint32_t>(29u);

		struct PC
		{
			Matrix4f matrix;
			uint32_t flags;
			float line_count_f;
			float oo_line_count_minus_one;
		};

		const uint32_t lc = 2 * line_count + 1;
		
		PC pc{
			.matrix = camera.getWorldToProj(),
			.flags = _flags,
			.line_count_f = float(lc),
			.oo_line_count_minus_one = 1.0f / float(lc - 1),
		};

		exec(_render_planes->with(VertexCommand::SingleDrawInfo{
			.draw_count = 3,
			.instance_count = 1,
			.pc_data = &pc,
			.pc_size = sizeof(PC),
		}));

		exec(_render_lines->with(VertexCommand::SingleDrawInfo{
			.draw_count = lc * 3 * 2,
			.instance_count = 1,
			.pc_data = &pc,
			.pc_size = sizeof(PC),
		}));

		if (_framebuffer)
		{
			exec.endRenderPass();
		}
	}

	void RenderWorldBasis::declareGUI(GUI::Context& ctx)
	{
		ImGui::PushID(this);
		{
			uint32_t axis_mask = (_flags >> 29) & 0b111;
			uint32_t line_count = _flags & std::bitMask<uint32_t>(29);

			ImGui::Text("Show Axis:");
			for (char i = 0; i < 3; ++i)
			{
				ImGui::SameLine();
				const uint32_t bit = (0x1 << i);
				bool b = axis_mask & bit;
				char label[2] = {'x' + i, 0};
				if (ImGui::Checkbox(label, &b))
				{
					if (b)
					{
						axis_mask |= bit;
					}
					else
					{
						axis_mask &= ~bit;
					}
				}
			}

			ImGui::InputInt("Lines", reinterpret_cast<int*>(&line_count));

			_flags = axis_mask << 29 | (line_count & std::bitMask<uint32_t>(29));
		}
		ImGui::PopID();
	}
}