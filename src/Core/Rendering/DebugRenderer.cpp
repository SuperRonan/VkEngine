#include "DebugRenderer.hpp"
#include <Core/Execution/Executor.hpp>

#include <Core/Commands/PrebuiltTransferCommands.hpp>

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

	DebugRenderer::~DebugRenderer()
	{
		if (_debug_buffer)
		{
			_debug_buffer->removeInvalidationCallback(this);
		}
	}

	void DebugRenderer::declareCommonDefinitions()
	{
		if (_common_definitions)
		{
			auto& common_defs = *_common_definitions;
			common_defs.setDefinition("GLOBAL_ENABLE_GLSL_DEBUG", std::to_string(int(_enable_debug)));
			common_defs.setDefinition("SHADER_STRING_CAPACITY", std::to_string(_shader_string_capacity));
			common_defs.setDefinition("BUFFER_STRING_CAPACITY", std::to_string(_buffer_string_capacity));
			common_defs.setDefinition("GLYPH_SIZE", std::to_string(_default_glyph_size.index()));
			common_defs.setDefinition("DEFAULT_FLOAT_PRECISION", std::to_string(_default_float_precision) + "u");
			common_defs.setDefinition("DEFAULT_SHOW_PLUS", _default_show_plus ? "true"s : "false"s);

			if (_define_capacity)
			{
				common_defs.setDefinition("DEBUG_BUFFER_STRINGS_CAPACITY", std::to_string(_number_of_debug_strings));
				common_defs.setDefinition("DEBUG_BUFFER_LINES_CAPACITY", std::to_string(_number_of_debug_lines));
			}
		}
	}

	void DebugRenderer::createRenderShader()
	{
		const std::filesystem::path string_shaders = application()->mountingPoints()["ShaderLib"] + "Debug/RenderDebugStrings.glsl";
		const std::filesystem::path lines_shaders = application()->mountingPoints()["ShaderLib"] + "Debug/RenderDebugLines.glsl";

		std::vector<std::string> defs;
		using namespace std::containers_append_operators;

		const bool use_mesh_shader = (application()->availableFeatures().mesh_shader_ext.meshShader) && (application()->availableFeatures().mesh_shader_ext.taskShader);

		const VkCompareOp cmp = _depth ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS;

		if (use_mesh_shader)
		{
			defs += "MESH_PIPELINE 1"s;
			_render_strings_with_mesh = std::make_shared<MeshCommand>(MeshCommand::CI{
				.app = application(),
				.name = name() + ".RenderStrings",
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.bindings = {
					Binding{
						.image = _font->getView(),
						.sampler = _sampler,
						.binding = 0,
					},
				},
				.color_attachements = { _target },
				.depth_stencil = _depth,
				.write_depth = false,
				.depth_compare_op = cmp,
				.mesh_shader_path = string_shaders,
				.fragment_shader_path = string_shaders,
				.definitions = defs,
				.blending = Pipeline::BlendAttachementBlendingAlphaDefault(),
			});
			_render_lines_with_mesh = std::make_shared<MeshCommand>(MeshCommand::CI{
				.app = application(),
				.name = name() + ".RenderLines",
				.dispatch_threads = true,
				.line_raster_mode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
				.sets_layouts = _sets_layouts,
				.bindings = {},
				.color_attachements = { _target },
				.depth_stencil = _depth,
				.write_depth = false,
				.depth_compare_op = cmp,
				.mesh_shader_path = lines_shaders,
				.fragment_shader_path = lines_shaders,
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
						.image = _font->getView(),
						.sampler = _sampler,
						.binding = 0,
					},
				},
				.color_attachements = { _target },
				.depth_stencil = _depth,
				.write_depth = false,
				.depth_compare_op = cmp,
				.vertex_shader_path = string_shaders,
				.geometry_shader_path = string_shaders,
				.fragment_shader_path = string_shaders,
				.definitions = defs,
				.blending = Pipeline::BlendAttachementBlendingAlphaDefault(),
			});
			_render_lines_with_geometry = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderLines",
				.line_raster_mode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
				.sets_layouts = _sets_layouts,
				.bindings = {},
				.color_attachements = { _target },
				.depth_stencil = _depth,
				.write_depth = false,
				.depth_compare_op = cmp,
				.vertex_shader_path = lines_shaders,
				.geometry_shader_path = lines_shaders,
				.fragment_shader_path = lines_shaders,
				.definitions = defs,
				.blending = Pipeline::BlendAttachementBlendingAlphaDefault(),
			});
		}
	}

	void DebugRenderer::createResources()
	{
		const VkDeviceSize alignement = application()->deviceProperties().props2.properties.limits.minStorageBufferOffsetAlignment;
		const bool can_fp16 = application()->availableFeatures().features_12.shaderFloat16 && application()->availableFeatures().features_11.storageBuffer16BitAccess;
		const uint32_t string_meta_size = can_fp16 ? 12 : 20;
		Dyn<VkDeviceSize> buffer_header_strings_size = [this, alignement, string_meta_size]() {
			// Sizes in number of u32/f32
			const uint32_t header_size = 16;
			const uint32_t string_content_size = _buffer_string_capacity;
			VkDeviceSize full_size = (header_size + _number_of_debug_strings * (string_meta_size + string_content_size)) * 4;
			full_size = std::alignUp(full_size, alignement);
			return full_size;
		};

		const uint32_t line_size = can_fp16 ? 16 : 20;
		Dyn<VkDeviceSize> buffer_lines_size = [this, alignement, line_size]() {
			VkDeviceSize full_size = (line_size * _number_of_debug_lines) * 4;
			full_size = std::alignUp(full_size, alignement);
			return full_size;
		};

		Dyn<VkDeviceSize> buffer_size = buffer_header_strings_size + buffer_lines_size;

		_debug_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".DebugBuffer",
			.size = buffer_size,
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.hold_instance = &_enable_debug,
		});

		_debug_buffer_header_and_strings = BufferSegment{
			.buffer = _debug_buffer,
			.range = [=](){return Buffer::Range{.begin = 0, .len = buffer_header_strings_size.value()}; },
		};	

		_debug_buffer_lines = BufferSegment{
			.buffer = _debug_buffer,
			.range = [=]() {return Buffer::Range{.begin = buffer_header_strings_size.value(), .len = buffer_lines_size.value()}; },
		};

		_debug_buffer->setInvalidationCallback(Callback{
			.callback = [this](){_should_write_header = true; },
			.id = this,
		});

		const std::filesystem::path font_path = application()->mountingPoints()["ShaderLib"] + "/16x16_linear.png";

		_font = std::make_shared<TextureFromFile>(TextureFromFile::CI{
			.app = application(),
			.name = name() + ".font",
			.path = font_path,
			.desired_format = VK_FORMAT_R8_UNORM,
			.synch = true,
			.mips = Texture::MipsOptions::None,
			.layers = 256,
		});

		const VkExtent3D extent = _font->getView()->image()->extent().value();
		_glyph_size.x = extent.width;
		_glyph_size.y = extent.height;

		_sampler = std::make_shared<Sampler>(Sampler::CI{
			.app = application(),
			.name = name() + ".Sampler",
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.max_anisotropy = application()->deviceProperties().props2.properties.limits.maxSamplerAnisotropy,
		});
		
	}

	//void DebugRenderer::loadFont(ExecutionRecorder& exec)
	//{
	//	//{
	//	//	img::Image<img::io::byte> font_with_new_layout(16, 16 * 256);
	//	//	for (size_t x = 0; x < 16; ++x)
	//	//	{
	//	//		for (size_t y = 0; y < 16; ++y)
	//	//		{
	//	//			for (size_t l = 0; l < 256; ++l)
	//	//			{
	//	//				font_with_new_layout(x, y + 16 * l) = host_font(x + (l % 16) * 16, y + (l / 16) * 16);
	//	//			}
	//	//		}
	//	//	}
	//	//	std::filesystem::path save_path = ENGINE_SRC_PATH "/Core/Modules/16x16_linear.png";
	//	//	img::io::write(font_with_new_layout, save_path, img::io::WriteInfo{.is_default = false, .magic_number = 2 });
	//	//}
	//	//UploadImage& uploader = application()->getPrebuiltTransferCommands().upload_image;
	//	//exec(uploader.with(UploadImage::UploadInfo{
	//	//	.src = ObjectView(host_font.data(), host_font.byteSize()),
	//	//	.dst = _font,
	//	//}));

	//	//UploadImage upload(UploadImage::CI{
	//	//	.app = application(),
	//	//	.name = name() + ".UploadBuffer",
	//	//	.src = ObjectView(host_font.data(), host_font.byteSize()),
	//	//	.dst = _font,
	//	//});
	//	//exec(upload);

	//	//_font_loaded = true;
	//}

	void DebugRenderer::setTargets(std::shared_ptr<ImageView> const& target, std::shared_ptr<ImageView> const& depth)
	{
		assert(!_target);
		_target = target;
		_depth = depth;
		createRenderShader();
	}

	void DebugRenderer::updateResources(UpdateContext& ctx)
	{
		_number_of_debug_strings = (1 << _log2_number_of_debug_strings);
		_number_of_debug_lines = (1 << _log2_number_of_debug_lines);
		
		_font->updateResources(ctx);
		_sampler->updateResources(ctx);
		_debug_buffer->updateResource(ctx);


		if (_should_write_header && _debug_buffer->instance())
		{
			struct Header {
				uint32_t num_debug_strings;
				uint32_t num_debug_lines;
				uint32_t pad1;
				uint32_t pad2;
			};
			Header header{
				.num_debug_strings = _number_of_debug_strings,
				.num_debug_lines = _number_of_debug_lines,
			};	
			ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
				.sources = {
					PositionedObjectView{
						.obj = header,
						.pos = 0,
					},
				},
				.dst = _debug_buffer->instance(),
			};
			_should_write_header = false;
		}

		if (_render_strings_with_geometry)
		{
			ctx.resourcesToUpdateLater() += _render_strings_with_geometry;
			ctx.resourcesToUpdateLater() += _render_lines_with_geometry;
		}
		if (_render_strings_with_mesh)
		{
			ctx.resourcesToUpdateLater() += _render_strings_with_mesh;
			ctx.resourcesToUpdateLater() += _render_lines_with_mesh;
		}
	}

	void DebugRenderer::execute(ExecutionRecorder& exec)
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
			if (_render_strings_with_mesh)
			{
				MeshCommand::DrawInfo draw_info{
					.draw_type = DrawType::Draw,
					.dispatch_threads = true,
					.draw_list = {
						MeshCommand::DrawCallInfo{
							.pc = pc,
							.extent = {.width = _number_of_debug_strings, .height = 1, .depth = 1 },
						},
					},
				};
				exec(_render_strings_with_mesh->with(draw_info));
				draw_info.draw_list.front().extent.width = _number_of_debug_lines;
				exec(_render_lines_with_mesh->with(draw_info));
			}
			else
			{
				VertexCommand::SingleDrawInfo draw_info{
					.draw_count = _number_of_debug_strings,
					.pc = pc,
				};
				exec(_render_strings_with_geometry->with(draw_info));
				draw_info.draw_count = _number_of_debug_lines;
				exec(_render_lines_with_geometry->with(draw_info));
			}


			Buffer::Range clear_range = _debug_buffer->instance()->fullRange();
			const size_t excluded = 4 * sizeof(uint32_t); // Don't fill the first part of the header
			clear_range.begin += excluded;
			clear_range.len -= excluded;
			exec(application()->getPrebuiltTransferCommands().fill_buffer.with(FillBuffer::FillInfo{
				.buffer = _debug_buffer,
				.range = clear_range,
				.value = 0,
			}));
		}
	}

	void DebugRenderer::declareGui(GuiContext & ctx)
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

				changed = ImGui::Checkbox("#define capacities", &_define_capacity);
				if (changed)
				{
					if (_define_capacity)
					{
						common_defs.setDefinition("DEBUG_BUFFER_STRINGS_CAPACITY", std::to_string(_number_of_debug_strings));
						common_defs.setDefinition("DEBUG_BUFFER_LINES_CAPACITY", std::to_string(_number_of_debug_lines));
					}
					else
					{
						common_defs.removeDefinition("DEBUG_BUFFER_STRINGS_CAPACITY");
						common_defs.removeDefinition("DEBUG_BUFFER_LINES_CAPACITY");
					}
				}

				changed = ImGui::InputInt("log2(Total Strings Capacity)", (int*) & _log2_number_of_debug_strings);
				if (changed)
				{
					_should_write_header = true;
					_log2_number_of_debug_strings = std::max<int>(_log2_number_of_debug_strings, 0);
					_number_of_debug_strings = (1 << _log2_number_of_debug_strings);
					if(_define_capacity)	common_defs.setDefinition("DEBUG_BUFFER_STRINGS_CAPACITY", std::to_string(_number_of_debug_strings));
				}
				ImGui::Text("Total Strings Capacity: %d", _number_of_debug_strings);
				
				changed = ImGui::InputInt("log2(Total Lines Capacity)", (int*)&_log2_number_of_debug_lines);
				if (changed)
				{
					_should_write_header = true;
					_log2_number_of_debug_lines = std::max<int>(_log2_number_of_debug_lines, 0);
					_number_of_debug_lines = (1 << _log2_number_of_debug_lines);
					if (_define_capacity)	common_defs.setDefinition("DEBUG_BUFFER_LINES_CAPACITY", std::to_string(_number_of_debug_lines));
				}
				ImGui::Text("Total Lines Capacity: %d", _number_of_debug_lines);
			}

			size_t size = 0;
			if (_enable_debug)
			{
				size = _debug_buffer->size().value();
			}
			if (size < 1024)
			{
				ImGui::Text("DebugBuffer memory consumption: %dB", size);
			}
			else
			{
				uint32_t i = -1;
				double dec;
				const char * units[3] = {"KiB", "MiB", "GiB"};
				do
				{
					dec = size / 1024.0;
					size /= 1024;
					++i;
				} while(size > 1024 && i < 3);
				ImGui::Text("DebugBuffer memory consumption: %.3f %s", dec, units[i]);
			}
		}
	}

	ShaderBindings DebugRenderer::getBindings()const
	{
		ShaderBindings res = {
			Binding{
				.buffer = _debug_buffer_header_and_strings,
				.binding = 0,
			},
			Binding{
				.buffer = _debug_buffer_lines,
				.binding = 1,
			},
		};
		return res;
	}

	MyVector<DescriptorSetLayout::Binding> DebugRenderer::getLayoutBindings(uint32_t offset)
	{
		MyVector<DescriptorSetLayout::Binding> res;
		res += DescriptorSetLayout::Binding{
			.name = "DebugBufferStrings",
			.binding = offset + 0,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		};
		res += DescriptorSetLayout::Binding{
			.name = "DebugBufferLines",
			.binding = offset + 1,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		};
		return res;
	}
}

