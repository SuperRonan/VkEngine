#include <vkl/Rendering/DebugRenderer.hpp>
#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include "imgui.h"

#include <that/img/ImRead.hpp>
#include <that/img/ImWrite.hpp>

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
			common_defs.setDefinition("GLOBAL_ENABLE_SHADER_DEBUG", std::to_string(int(_enable_debug)));
			common_defs.setDefinition("SHADER_STRING_CAPACITY", std::to_string(_shader_string_capacity));
			common_defs.setDefinition("GLYPH_SIZE", std::to_string(int(_default_glyph_size.index()) - 2));
			common_defs.setDefinition("DEFAULT_FLOAT_PRECISION", std::to_string(_default_float_precision) + "u");
			common_defs.setDefinition("DEFAULT_SHOW_PLUS", _default_show_plus ? "true"s : "false"s);

			if (_define_capacity)
			{
				common_defs.setDefinition("DEBUG_BUFFER_STRINGS_CAPACITY", std::to_string(_number_of_debug_strings));
				common_defs.setDefinition("DEBUG_BUFFER_STRINGS_CONTENT_CAPACITY", std::to_string(_debug_chunks_capacity));
				common_defs.setDefinition("DEBUG_BUFFER_LINES_CAPACITY", std::to_string(_number_of_debug_lines));
			}
		}
	}

	void DebugRenderer::createRenderShader()
	{
		const std::filesystem::path shaders = "ShaderLib:/Debug/";
		const std::filesystem::path string_shaders = shaders / "RenderDebugStrings.glsl";
		const std::filesystem::path lines_shaders = shaders / "RenderDebugLines.glsl";

		DefinitionsList defs;
		using namespace std::containers_append_operators;

		const bool use_mesh_shader = (application()->availableFeatures().mesh_shader_ext.meshShader) && (application()->availableFeatures().mesh_shader_ext.taskShader);

		const VkCompareOp cmp = _depth ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS;

		GraphicsPipeline::LineRasterizationState line_raster_state{
			.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT
		};

		_render_pass = std::make_shared<RenderPass>(RenderPass::SPCI{
			.app = application(),
			.name = name() + ".RenderPass",
			.colors = {AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Blend, _target)},
			.depth_stencil = AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::ReadOnly, _depth),
		});

		Framebuffer::CI fb_ci{
			.app = application(),
			.name = name() + ".Framebuffer",
			.render_pass = _render_pass,
			.attachments = {_target},
		};
		if (_depth)
		{
			fb_ci.attachments.push_back(_depth);
		}
		_framebuffer = std::make_shared<Framebuffer>(std::move(fb_ci));

		_blending = AttachmentBlending::DefaultAlphaBlending();

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
				.extern_render_pass = _render_pass,
				.color_attachments = {
					GraphicsCommand::ColorAttachment{
						.blending = &_blending,
					}
				},
				.write_depth = false,
				.depth_compare_op = cmp,
				.mesh_shader_path = string_shaders,
				.fragment_shader_path = string_shaders,
				.definitions = defs,
			});
			_render_lines_with_mesh = std::make_shared<MeshCommand>(MeshCommand::CI{
				.app = application(),
				.name = name() + ".RenderLines",
				.dispatch_threads = true,
				.line_raster_state = line_raster_state,
				.sets_layouts = _sets_layouts,
				.bindings = {},
				.extern_render_pass = _render_pass,
				.color_attachments = {
					GraphicsCommand::ColorAttachment{
						.blending = &_blending,
					}
				},
				.write_depth = false,
				.depth_compare_op = cmp,
				.mesh_shader_path = lines_shaders,
				.fragment_shader_path = lines_shaders,
				.definitions = defs,
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
				.extern_render_pass = _render_pass,
				.color_attachments = {
					GraphicsCommand::ColorAttachment{
						.blending = &_blending,
					}
				},
				.write_depth = false,
				.depth_compare_op = cmp,
				.vertex_shader_path = string_shaders,
				.geometry_shader_path = string_shaders,
				.fragment_shader_path = string_shaders,
				.definitions = defs,
			});
			_render_lines_with_geometry = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = application(),
				.name = name() + ".RenderLines",
				.line_raster_state = line_raster_state,
				.sets_layouts = _sets_layouts,
				.bindings = {},
				.extern_render_pass = _render_pass,
				.color_attachments = {
					GraphicsCommand::ColorAttachment{
						.blending = &_blending,
					}
				},
				.write_depth = false,
				.depth_compare_op = cmp,
				.vertex_shader_path = lines_shaders,
				.geometry_shader_path = lines_shaders,
				.fragment_shader_path = lines_shaders,
				.definitions = defs,
			});
		}
	}

	void DebugRenderer::createResources()
	{
		const VkDeviceSize alignement = application()->deviceProperties().props2.properties.limits.minStorageBufferOffsetAlignment;
		const bool can_fp16 = application()->availableFeatures().features_12.shaderFloat16 && application()->availableFeatures().features_11.storageBuffer16BitAccess;
		const uint32_t string_meta_size = can_fp16 ? 12 : 20;
		const uint32_t header_size = std::alignUp<uint32_t>(16 * 4, alignement);
		
		Dyn<VkDeviceSize> buffer_string_meta_size = [this, alignement, string_meta_size]() {
			// Sizes in number of u32/f32
			VkDeviceSize full_size = (_number_of_debug_strings * (string_meta_size)) * 4;
			full_size = std::alignUp(full_size, alignement);
			return full_size;
		};

		Dyn<VkDeviceSize> buffer_string_content_size = [this, alignement]()
		{
			VkDeviceSize full_size = (_debug_chunks_capacity);
			full_size = std::alignUp(full_size, alignement);
			return full_size;
		};

		const uint32_t line_size = can_fp16 ? 16 : 20;
		Dyn<VkDeviceSize> buffer_lines_size = [this, alignement, line_size]() {
			VkDeviceSize full_size = (line_size * _number_of_debug_lines) * 4;
			full_size = std::alignUp(full_size, alignement);
			return full_size;
		};

		Dyn<VkDeviceSize> buffer_size = [=](){return VkDeviceSize(header_size) + *buffer_string_meta_size + *buffer_string_content_size + *buffer_lines_size; };

		_debug_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".DebugBuffer",
			.size = buffer_size,
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.hold_instance = &_enable_debug,
		});

		_debug_buffer_header = BufferSegment{
			.buffer = _debug_buffer,
			.range = [=](){return Buffer::Range{.begin = 0, .len = header_size}; },
		};	

		_debug_buffer_strings_meta = BufferSegment{
			.buffer = _debug_buffer,
			.range = [=](){return Buffer::Range{.begin = header_size, .len = buffer_string_meta_size.value()};}
		};

		_debug_buffer_strings_content = BufferSegment{
			.buffer = _debug_buffer,
			.range = [=](){
				Buffer::Range prev_range = _debug_buffer_strings_meta.range.value();
				return Buffer::Range{.begin = prev_range.end(), .len = buffer_string_content_size.value()};
			},
		};

		_debug_buffer_lines = BufferSegment{
			.buffer = _debug_buffer,
			.range = [=]() {
				Buffer::Range prev_range = _debug_buffer_strings_content.range.value();
				return Buffer::Range{.begin = prev_range.end(), .len = buffer_lines_size.value()};  
			},
		};

		_debug_buffer->setInvalidationCallback(Callback{
			.callback = [this](){_should_write_header = true; },
			.id = this,
		});

		const std::filesystem::path font_path = "ShaderLib:/16x16_linear.png";

		_font = std::make_shared<TextureFromFile>(TextureFromFile::CI{
			.app = application(),
			.name = name() + ".font",
			.path = font_path,
			.desired_format = VK_FORMAT_R8_UNORM,
			.synch = true,
			.mips = Texture::MipsOptions::None,
			.layers = 256,
		});

		if (_font->getView())
		{
			const VkExtent3D extent = _font->getView()->image()->extent().value();
			_glyph_size = {extent.width, extent.height};
		}
		else
		{
			_enable_debug = false;
		}

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
		_debug_chunks_capacity = (1 << _log2_debug_chunks);
		_debug_buffer->updateResource(ctx); // this one holds instance based on _enable_debug
		if (_enable_debug)
		{
			_font->updateResources(ctx);
			_sampler->updateResources(ctx);


			if (_should_write_header && _debug_buffer->instance())
			{
				struct Header {
					uint32_t num_debug_strings;
					uint32_t debug_chunk_capacity;
					uint32_t num_debug_lines;
					uint32_t pad;
				};
				Header header{
					.num_debug_strings = _number_of_debug_strings,
					.debug_chunk_capacity = _debug_chunks_capacity,
					.num_debug_lines = _number_of_debug_lines,
				};	
				ResourcesToUpload::BufferSource src{
					.data = &header,
					.size = sizeof(header),
					.copy_data = true,
				};
				ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
					.sources = &src,
					.sources_count = 1,
					.dst = _debug_buffer->instance(),
				};
				_should_write_header = false;
			}

			_render_pass->updateResources(ctx);
			ctx.resourcesToUpdateLater() += _framebuffer;

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
	}

	void DebugRenderer::execute(ExecutionRecorder& exec)
	{
		if (_enable_debug)
		{
			struct PC 
			{
				alignas(16) Vector3u resolution;
				alignas(16) Vector2f oo_resolution;
			};
			const VkExtent3D ext = *_target->image()->extent();
			PC pc = {
				.resolution = Vector3u(ext.width, ext.height, 1),
				.oo_resolution = Vector2f(1.0 / float(ext.width), 1.0 / float(ext.height)),
			};

			RenderPassBeginInfo render_pass{
				.framebuffer = _framebuffer->instance(),
			};

			exec.beginRenderPass(render_pass);

			if (_render_strings_with_mesh)
			{
				MeshCommand::DrawInfo draw_info{
					.draw_type = DrawType::Draw,
					.dispatch_threads = true,
				};
				draw_info.pushBack(MeshCommand::DrawCallInfo{
					.pc_data = &pc,
					.pc_size = sizeof(pc),
					.extent = {.width = _number_of_debug_strings, .height = 1, .depth = 1 },
				});
				exec(_render_strings_with_mesh->with(draw_info));
				draw_info.draw_list.front().extent.width = _number_of_debug_lines;
				exec(_render_lines_with_mesh->with(draw_info));
			}
			else
			{
				VertexCommand::SingleDrawInfo draw_info{
					.draw_count = _number_of_debug_strings,
					.pc_data = &pc,
					.pc_size = sizeof(pc),
				};
				exec(_render_strings_with_geometry->with(draw_info));
				draw_info.draw_count = _number_of_debug_lines;
				exec(_render_lines_with_geometry->with(draw_info));
			}

			exec.endRenderPass();


			// Clear only the atomic counters
			Buffer::Range clear_range;
			const size_t header_size = 4 * sizeof(u32);
			if (true) // only clear counters
			{
				clear_range = { .begin = header_size, .len = 3 * header_size };
			}
			else
			{
				clear_range = _debug_buffer->fullRange().value();
				clear_range.begin += header_size;
				clear_range.len -= header_size;

			}
			exec(application()->getPrebuiltTransferCommands().fill_buffer.with(FillBuffer::FillInfo{
				.buffer = _debug_buffer,
				.range = clear_range,
				.value = 0,
			}));
		}
	}

	void DebugRenderer::declareGui(GuiContext & ctx)
	{
		if (ImGui::CollapsingHeader("Shader Debugging"))
		{
			if (_common_definitions)
			{
				auto& common_defs = *_common_definitions;
				bool changed = false;
			
				changed = ImGui::Checkbox("Enable", &_enable_debug);
				if (changed)
				{
					common_defs.setDefinition("GLOBAL_ENABLE_SHADER_DEBUG", std::to_string(int(_enable_debug)));
				}
			
				changed = ImGui::SliderInt("Shader String Chunks", & _shader_string_chunks, 1, 16);
				if (changed)
				{
					_shader_string_capacity = 4 * _shader_string_chunks;
					common_defs.setDefinition("SHADER_STRING_CAPACITY", std::to_string(_shader_string_capacity));
				}

				
				changed = _default_glyph_size.declare();
				if (changed)
				{
					common_defs.setDefinition("GLYPH_SIZE", std::to_string(int(_default_glyph_size.index()) - 2));
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

				changed = ImGui::InputInt("log2(Total Strings Content Capcity)", (int*)&_log2_debug_chunks);
				if (changed)
				{
					_should_write_header = true;
					_log2_debug_chunks = std::max<int>(_log2_debug_chunks, 0);
					_debug_chunks_capacity = (1 << _log2_debug_chunks);
					if(_define_capacity)	common_defs.setDefinition("DEBUG_BUFFER_STRINGS_CONTENT_CAPACITY", std::to_string(_debug_chunks_capacity));
				}
				ImGui::Text("Total Strings Content Capacity %dB", _debug_chunks_capacity);
				
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
				.buffer = _debug_buffer_header,
				.binding = 0,
			},
			Binding{
				.buffer = _debug_buffer_strings_meta,
				.binding = 1,
			},
			Binding{
				.buffer = _debug_buffer_strings_content,
				.binding = 2,
			},
			Binding{
				.buffer = _debug_buffer_lines,
				.binding = 3,
			},
		};
		return res;
	}

	MyVector<DescriptorSetLayout::Binding> DebugRenderer::getLayoutBindings(uint32_t offset)
	{
		MyVector<DescriptorSetLayout::Binding> res;
		res += DescriptorSetLayout::Binding{
			.name = "DebugBufferHeader",
			.binding = offset + 0,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		};
		res += DescriptorSetLayout::Binding{
			.name = "DebugBufferStringsMeta",
			.binding = offset + 1,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		};
		res += DescriptorSetLayout::Binding{
			.name = "DebugBufferStringsContent",
			.binding = offset + 2,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		};
		res += DescriptorSetLayout::Binding{
			.name = "DebugBufferLines",
			.binding = offset + 3,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		};
		return res;
	}
}

