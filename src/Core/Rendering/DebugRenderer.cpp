#include "DebugRenderer.hpp"
#include <Core/Execution/Executor.hpp>

#include "imgui.h"

#include <thatlib/src/img/ImWrite.hpp>

namespace vkl
{
	DebugRenderer::DebugRenderer(CreateInfo const& ci) :
		Module(ci.app, ci.name.empty() ? "DebugRenderer" : ci.name),
		_common_definitions(ci.common_definitions),
		_sets_layouts(ci.sets_layout),
		_target(ci.target),
		_depth(ci.depth)
	{

		_default_glyph_size = ImGuiListSelection::CI{
			.name = "Font size",
			.mode = ImGuiListSelection::Mode::Dropdown,
			.labels = {
				"Tiny",
				"Small",
				"Normal",
				"Large",
				"Huge",
			},
			.default_index = 2,
		};
		
		createResources();
		
		declareCommonDefinitions();

		if (_target)
		{
			createRenderShader();
		}
	}

	void DebugRenderer::declareCommonDefinitions()
	{
		if (_common_definitions)
		{
			auto& common_defs = *_common_definitions;
			common_defs.setDefinition("GLOBAL_ENABLE_GLSL_DEBUG", std::to_string(int(_enable_debug)));
			common_defs.setDefinition("SHADER_STRING_CAPACITY", std::to_string(_shader_string_capacity));
			common_defs.setDefinition("DEBUG_BUFFER_STRING_SIZE", std::to_string(_number_of_debug_strings));
			common_defs.setDefinition("BUFFER_STRING_CAPACITY", std::to_string(_buffer_string_capacity));
			common_defs.setDefinition("GLYPH_SIZE", std::to_string(_default_glyph_size.index()));
			common_defs.setDefinition("DEFAULT_FLOAT_PRECISION", std::to_string(_default_float_precision) + "u");
			common_defs.setDefinition("DEFAULT_SHOW_PLUS", _default_show_plus ? "true"s : "false"s);
		}
	}

	void DebugRenderer::createRenderShader()
	{
		const std::filesystem::path shaders = ENGINE_SRC_PATH "/Shaders/RenderDebugStrings.glsl";

		std::vector<std::string> defs;
		using namespace std::containers_operators;

		if (application()->availableFeatures().mesh_shader_ext.meshShader)
		{
			defs += "MESH_PIPELINE 1"s;
			_render_strings_with_mesh = std::make_shared<MeshCommand>(MeshCommand::CI{
				.app = application(),
				.name = name() + ".RenderStrings",
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.view = _font,
						.sampler = _sampler,
						.binding = 0,
					},
				},
				.color_attachements = { _target },
				.depth_buffer = _depth,
				.write_depth = false,
				.mesh_shader_path = shaders,
				.fragment_shader_path = shaders,
				.definitions = defs,
				.blending = Pipeline::BlendAttachementBlendingAlphaDefault(),
			});
		}
		else
		{
			defs += "GEOMETRY_PIPELINE 1"s;
			_render_strings_with_geometry = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderStrings",
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.view = _font,
						.sampler = _sampler,
						.binding = 0,
					},
				},
				.color_attachements = { _target },
				.depth_buffer = _depth,
				.write_depth = false,
				.vertex_shader_path = shaders,
				.geometry_shader_path = shaders,
				.fragment_shader_path = shaders,
				.definitions = defs,
				.blending = Pipeline::BlendAttachementBlendingAlphaDefault(),
			});

		}
	}

	void DebugRenderer::createResources()
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
		Dyn<VkDeviceSize> buffer_size = [&]() {
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


		_clear_buffer = std::make_shared<FillBuffer>(FillBuffer::CI{
			.app = application(),
			.name = name() + ".ClearBuffer",
			.buffer = _string_buffer,
		});

		_font = std::make_shared<ImageView>(ImageView::CI{
			.app = application(),
			.name = name() + ".Font",
			.image_ci = Image::CI{
				.app = application(),
				.name = name() + ".Font",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8_UNORM,
				.extent = [&](){return VkExtent3D{.width = _glyph_size.x, .height = _glyph_size.y, .depth = 1};},
				.mips = 1,
				.layers = 256,
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			},
			.type = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		});


		_sampler = std::make_shared<Sampler>(Sampler::CI{
			.app = application(),
			.name = name() + ".Sampler",
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.max_anisotropy = application()->deviceProperties().props.limits.maxSamplerAnisotropy,
		});
		
	}

	void DebugRenderer::loadFont(ExecutionThread& exec)
	{
		img::Image<img::io::byte> host_font = img::io::read<img::io::byte>(ENGINE_SRC_PATH "/Core/Rendering/16x16_linear.png");

		//{
		//	img::Image<img::io::byte> font_with_new_layout(16, 16 * 256);
		//	for (size_t x = 0; x < 16; ++x)
		//	{
		//		for (size_t y = 0; y < 16; ++y)
		//		{
		//			for (size_t l = 0; l < 256; ++l)
		//			{
		//				font_with_new_layout(x, y + 16 * l) = host_font(x + (l % 16) * 16, y + (l / 16) * 16);
		//			}
		//		}
		//	}
		//	std::filesystem::path save_path = ENGINE_SRC_PATH "/Core/Modules/16x16_linear.png";
		//	img::io::write(font_with_new_layout, save_path, img::io::WriteInfo{.is_default = false, .magic_number = 2 });
		//}

		UploadImage upload(UploadImage::CI{
			.app = application(),
			.name = name() + ".UploadBuffer",
			.src = ObjectView(host_font.data(), host_font.byteSize()),
			.dst = _font,
		});

		exec(upload);

		_font_loaded = true;
	}

	void DebugRenderer::setTargets(std::shared_ptr<ImageView> const& target, std::shared_ptr<ImageView> const& depth)
	{
		assert(!_target);
		_target = target;
		_depth = depth;
		createRenderShader();
	}

	void DebugRenderer::updateResources(UpdateContext& ctx)
	{
		_font->updateResource(ctx);
		_sampler->updateResources(ctx);
		_string_buffer->updateResource(ctx);

		if (_render_strings_with_geometry)
		{
			ctx.resourcesToUpdateLater() += _render_strings_with_geometry;
		}
		if (_render_strings_with_mesh)
		{
			ctx.resourcesToUpdateLater() += _render_strings_with_mesh;
		}
	}

	void DebugRenderer::execute(ExecutionThread& exec)
	{
		if (_enable_debug)
		{

			if (!_font_loaded)
			{
				loadFont(exec);
			}

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
			if (_render_strings_with_mesh)
			{
				VkExtent3D extent = { .width = _number_of_debug_strings, .height = 1, .depth = 1 };
				exec(_render_strings_with_mesh->with(MeshCommand::DrawInfo{
					.draw_type = MeshCommand::DrawType::Draw,
					.dispatch_threads = true,
					.draw_list = {
						MeshCommand::DrawCallInfo{
							.pc = pc,
							.extent = extent,
						},
					}
				}));
			}
			else
			{
				exec(_render_strings_with_geometry->with(VertexCommand::DrawInfo{
					.draw_type = GraphicsCommand::DrawType::Draw,
					.draw_list = {
						VertexCommand::DrawCallInfo{
							.draw_count = _number_of_debug_strings,
							.pc = pc,
						},
					},
				}));
			}

			exec(_clear_buffer);
		}
	}

	void DebugRenderer::declareImGui()
	{
		if (ImGui::CollapsingHeader("GLSL Debug"))
		{
			if (_common_definitions)
			{
				auto& common_defs = *_common_definitions;
				bool changed = false;
			
				changed = ImGui::Checkbox("Enable", &_enable_debug);
				if (changed)
				{
					common_defs.setDefinition("GLOBAL_ENABLE_GLSL_DEBUG", std::to_string(int(_enable_debug)));
				}
			
				changed = ImGui::SliderInt("Shader String Chunks", & _shader_string_chunks, 1, _buffer_string_capacity / 4);
				if (changed)
				{
					_shader_string_capacity = 4 * _shader_string_chunks;
					common_defs.setDefinition("SHADER_STRING_CAPACITY", std::to_string(_shader_string_capacity));
				}

				changed = ImGui::SliderInt("Buffer String Chunks", &_buffer_string_chunks, 1, 32);
				if (changed)
				{
					_buffer_string_capacity = 4 * _buffer_string_chunks;
					common_defs.setDefinition("BUFFER_STRING_CAPACITY", std::to_string(_buffer_string_capacity));
				}

				
				changed = _default_glyph_size.declare();
				if (changed)
				{
					common_defs.setDefinition("GLYPH_SIZE", std::to_string(_default_glyph_size.index()));
				}

				changed = ImGui::SliderInt("Float precision", &_default_float_precision, 1, 12);
				if (changed)
				{
					common_defs.setDefinition("DEFAULT_FLOAT_PRECISION", std::to_string(_default_float_precision) + "u");
				}

				changed = ImGui::Checkbox("Always print +", &_default_show_plus);
				if (changed)
				{
					common_defs.setDefinition("DEFAULT_SHOW_PLUS", _default_show_plus ? "true"s : "false"s);
				}
			}
		}
	}

	ShaderBindings DebugRenderer::getBindings()const
	{
		ShaderBindings res = {
			Binding{
				.buffer = _string_buffer,
				.binding = 0,
			},
		};
		return res;
	}
}

