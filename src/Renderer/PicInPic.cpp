#include "PicInPic.hpp"
#include <Core/Execution/Executor.hpp>
#include "imgui.h"

namespace vkl
{
	PictureInPicture::PictureInPicture(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_target(ci.target),
		_exec(ci.exec),
		_sets_layouts(ci.sets_layouts)
	{
		const std::filesystem::path common_shaders = ENGINE_SRC_PATH "/Shaders/";

		_definitions = [&]() -> std::vector<std::string> {
			DetailedVkFormat detailed_format = DetailedVkFormat::Find(_target->format());
			std::string glsl_format = detailed_format.getGLSLName();
			return {glsl_format, };
		};

		_fast_pip = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".FastPiP",
			.shader_path = common_shaders / "FastPiP.comp",
			.dispatch_size = _target->image()->extent(),
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.view = _target,
					.binding = 0,
				},
			},
			.definitions = _definitions,
		});
		_exec.declare(_fast_pip);

		_show_outline = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".ShowOutline",
			.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
			.line_raster_mode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
			.sets_layouts = _sets_layouts,
			.color_attachements = {_target},
			.vertex_shader_path = common_shaders / "RenderRect.glsl",
			.fragment_shader_path = common_shaders / "RenderRect.glsl",
		});
		_exec.declare(_show_outline);
	}

	void PictureInPicture::execute()
	{
		if (_enable)
		{
			const float region_size = _pip_size / _zoom;
			const Vector2f pip_pos = _pip_position - 0.5f * Vector2f(region_size);
			FastPiPPC pc{
				.zoom = _zoom,
				.pip_size = _pip_size,
				.pip_pos = pip_pos,
			};

			_exec(_fast_pip->with(ComputeCommand::DispatchInfo{
				.push_constant = pc,
			}));

			
			_exec(_show_outline->with(VertexCommand::DrawInfo{
				.draw_type = GraphicsCommand::DrawType::Draw,
				.draw_list = {
					VertexCommand::DrawCallInfo{
						.draw_count = 5,
						.pc = ShowOutlinePC{
							.pos = pip_pos,
							.size = Vector2f(region_size),
							.color = Vector4f(1, 1, 1, 1),
						},
					},
					VertexCommand::DrawCallInfo{
						.draw_count = 5,
						.pc = ShowOutlinePC{
							.pos = Vector2f(0),
							.size = Vector2f(_pip_size),
							.color = Vector4f(1, 1, 1, 1),
						},
					},
				},
			}));
		}
	}

	void PictureInPicture::declareImGui()
	{
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			ImGui::Checkbox("enable", &_enable);
			ImGui::SliderFloat("zoom", &_zoom, 1, 16);
			ImGui::SliderFloat("size", &_pip_size, 0, 1);
			ImGui::SliderFloat2("position", &_pip_position.x, 0, 1);
		}
	}
}