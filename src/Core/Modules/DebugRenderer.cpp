#include "DebugRenderer.hpp"

#include "imgui.h"

namespace vkl
{
	DebugRenderer::DebugRenderer(CreateInfo const& ci) :
		Module(ci.exec.application(), "DebugRenderer"),
		_exec(ci.exec),
		_target(ci.target)
	{
		//struct BufferStringMeta
		//{
		//	vec2 position;
		//	uint layer;
		//	uint len;
		//	vec2 glyph_size;
		//	vec4 color;
		//	vec4 back_color;
		//	uint flags; 
		//};
		// Larger than reality to be sure
		dv_<VkDeviceSize> buffer_size = [&]() {
			const uint32_t meta_size = (2 + 1 + 1 + 2 + 4 + 4 + 1) * 2;
			const VkDeviceSize full_size = _number_of_debug_strings * (meta_size + _buffer_string_capacity) * 4 + 16;
			return _enable_debug ? full_size : 256;
		};

		_string_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".DebugStringBuffer",
			.size = buffer_size,
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		_exec.declare(_string_buffer);

		_clear_buffer = std::make_shared<FillBuffer>(FillBuffer::CI{
			.app = application(),
			.name = name() + ".ClearBuffer",
			.buffer = _string_buffer,
		});
		_exec.declare(_clear_buffer);

		const std::filesystem::path shaders = ENGINE_SRC_PATH "/Shaders/RenderDebugStrings.glsl";

		using namespace std::containers_operators;

		_exec.getCommonBindings() += Binding{
			.buffer = _string_buffer,
			.set = _desc_set,
			.binding = _first_binding,
		};

		_render_strings = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".RenderStrings",
			.draw_count = &_number_of_debug_strings,
			.bindings = {
			},
			.color_attachements = {_target},
			.write_depth = false,
			.vertex_shader_path = shaders,
			.geometry_shader_path = shaders,
			.fragment_shader_path = shaders,
		});
		_exec.declare(_render_strings);

		auto& common_defs = _exec.getCommonDefinitions();
		common_defs.setDefinition("GLOBAL_ENABLE_GLSL_DEBUG", std::to_string(int(_enable_debug)));
		common_defs.setDefinition("DEBUG_BUFFER_BINDING", "set = " + std::to_string(_desc_set) + ", binding = " + std::to_string(_first_binding));
	}

	void DebugRenderer::execute()
	{
		if (_enable_debug)
		{
			struct PC 
			{
				alignas(16) glm::uvec3 resolution;
				alignas(16) glm::vec2 oo_resolution;
			};
			const VkExtent3D ext = *_target->image()->extent();
			PC pc = {
				.resolution = glm::uvec3(ext.width, ext.height, 1),
				.oo_resolution = glm::vec2(1.0 / float(ext.width), 1.0 / float(ext.height)),
			};
			_exec(_render_strings->with(VertexCommand::DrawInfo{
				.pc = pc,
			}));

			_exec(_clear_buffer);
		}
	}

	void DebugRenderer::declareToImGui()
	{
		if (ImGui::CollapsingHeader("GLSL Debug"))
		{
			const bool changed = ImGui::Checkbox("Enable", &_enable_debug);
			if (changed)
			{
				auto& common_defs = _exec.getCommonDefinitions();
				common_defs.setDefinition("GLOBAL_ENABLE_GLSL_DEBUG", std::to_string(int(_enable_debug)));
			}
		}
	}
}

