#include "PicInPic.hpp"
#include <vkl/Execution/Executor.hpp>
#include "imgui.h"

namespace vkl
{
	PictureInPicture::PictureInPicture(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_target(ci.target),
		_sets_layouts(ci.sets_layouts)
	{
		const std::filesystem::path common_shaders = "ShaderLib:/";

		_definitions = [this](DefinitionsList& res){
			res.clear();
			res += _glsl_format;
		};

		_fast_pip = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".FastPiP",
			.shader_path = common_shaders / "FastPiP.comp",
			.extent = _target->image()->extent(),
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.image = _target,
					.binding = 0,
				},
			},
			.definitions = _definitions,
		});
		GraphicsPipeline::LineRasterizationState line_raster_state{
			.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT
		};
		_show_outline = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".ShowOutline",
			.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
			.line_raster_state = line_raster_state,
			.sets_layouts = _sets_layouts,
			.color_attachments = { GraphicsCommand::ColorAttachment{.view = _target}},
			.vertex_shader_path = common_shaders / "RenderRect.glsl",
			.fragment_shader_path = common_shaders / "RenderRect.glsl",
		});
	}

	void PictureInPicture::updateResources(UpdateContext& context)
	{
		const bool update_anyway = context.updateAnyway();
		if (_enable || update_anyway)
		{
			DetailedVkFormat detailed_format = DetailedVkFormat::Find(_target->format().value());
			_glsl_format = detailed_format.getGLSLName();
			context.resourcesToUpdateLater() += _fast_pip;
			context.resourcesToUpdateLater() += _show_outline;
		}
	}

	void PictureInPicture::execute(ExecutionRecorder& exec)
	{
		if (_enable)
		{
			exec.pushDebugLabel(name(), true);
			const float region_size = _pip_size / _zoom;
			const Vector2f pip_pos = _pip_position - 0.5f * Vector2f::Constant(region_size);
			FastPiPPC pc{
				.zoom = _zoom,
				.pip_size = _pip_size,
				.pip_pos = pip_pos,
			};

			VkExtent3D extent = _target->image()->extent().value();
			Vector3f extent_f = Vector3f(extent.width, extent.height, extent.depth);
			extent_f = extent_f * _pip_size;
			extent = VkExtent3D{
				.width = static_cast<uint32_t>(std::ceil(extent_f.x())),
				.height = static_cast<uint32_t>(std::ceil(extent_f.y())),
				.depth = static_cast<uint32_t>(std::ceil(extent_f.z())),
			};

			exec(_fast_pip->with(ComputeCommand::SingleDispatchInfo{
				.extent = extent,
				.dispatch_threads = true,
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));

			static thread_local VertexCommand::DrawInfo vertex_draw_info;
			vertex_draw_info.clear();
			vertex_draw_info = VertexCommand::DrawInfo{
				.draw_type = DrawType::Draw,
			};
			ShowOutlinePC outline_pc{
				.pos = pip_pos,
				.size = Vector2f::Constant(region_size),
				.color = Vector4f(1, 1, 1, 1),
			};
			vertex_draw_info.pushBack(VertexCommand::DrawCallInfo{
				.pc_data = &outline_pc,
				.pc_size = sizeof(outline_pc),
				.draw_count = 5,
				.instance_count = 1,
			});
			outline_pc.pos = Vector2f::Zero();
			outline_pc.size = Vector2f::Constant(_pip_size);
			vertex_draw_info.pushBack(VertexCommand::DrawCallInfo{
				.pc_data = &outline_pc,
				.pc_size = sizeof(outline_pc),
				.draw_count = 5,
				.instance_count = 1,
			});
			exec(_show_outline->with(vertex_draw_info));
			exec.popDebugLabel();

			vertex_draw_info.clear();
		}
	}

	void PictureInPicture::declareGui(GuiContext & ctx)
	{
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			ImGui::Checkbox("enable", &_enable);
			ImGui::SliderFloat("zoom", &_zoom, 1, 16);
			ImGui::SliderFloat("size", &_pip_size, 0, 1);
			ImGui::SliderFloat2("position", _pip_position.data(), 0, 1);
		}
	}
}