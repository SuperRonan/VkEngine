#include "BSDFViewer.hpp"

#include <vkl/Execution/Executor.hpp>

#include <vkl/IO/ImGuiUtils.hpp>

#include <random>

namespace vkl
{
	struct UBOBase
	{
		Matrix4f world_to_proj;
		AffineXForm3Df world_to_camera;
		ubo_vec3 direction;
		float common_alpha;

		uint32_t reference_function;
		uint32_t seed;
		uint32_t pad1, pad2;

		float roughness;
		float metallic;
		float shininess;
		float pad3;
	};

	BSDFViewer::BSDFViewer(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_sets_layouts(ci.sets_layouts),
		_target(ci.target),
		_camera(ci.camera)
	{
		if (!_num_functions.hasValue())
		{
			_num_functions = 0;
		}
		if (!_sphere_resolution.hasValue())
		{
			_sphere_resolution = [this](){return VkExtent2D{.width = _resolution * 2, .height = _resolution}; };
		}

		_statistics = std::make_shared<Statistics>();

		createInternals();
	}

	BSDFViewer::~BSDFViewer()
	{
		_compute_statistics->pipeline()->removeInvalidationCallback(this);
	}

	void BSDFViewer::createInternals()
	{
		_resolution = 256;
		_alignment = 64;
		
		std::shared_ptr<Image> img = std::make_shared<Image>(Image::CI{
			.app = application(),
			.name = name() + ".BSDFImage",
			.type = VK_IMAGE_TYPE_2D,
			.format = &_format,
			.extent = [this]()
			{
				VkExtent2D a = _sphere_resolution.value();
				return VkExtent3D {.width = a.width, .height = a.height, .depth = 1};
			},
			.layers = [this](){return std::max(1u, _num_functions.value()); },
			.usage = VK_IMAGE_USAGE_STORAGE_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		_functions_image = std::make_shared<ImageView>(ImageView::CI{
			.app = application(),
			.name = name() + ".BSDFImage",
			.image = img,
			.type = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		});

		const size_t ubo_align = application()->deviceProperties().props2.properties.limits.minUniformBufferOffsetAlignment;
		const size_t ubo_size = std::alignUp(sizeof(UBOBase), ubo_align);
		_ubo = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + ".UBO",
			.size = ubo_size + 4 * sizeof(Vector4f),
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_statistics_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".StatisticsBuffer",
			.size = _num_functions * sizeof(FunctionStatistics),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_BITS,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.hold_instance = _num_functions > 0,
		});

		const bool can_mesh = application()->availableFeatures().mesh_shader_ext.meshShader && application()->availableFeatures().mesh_shader_ext.taskShader;

		_set_layout = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
			.app = application(),
			.name = name() + ".SetLayout",
			.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			.bindings = {
				DescriptorSetLayout::Binding{
					.name = "UBO",
					.binding = 0,
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.stages = VK_SHADER_STAGE_ALL,
					.access = VK_ACCESS_2_UNIFORM_READ_BIT,
					.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				},
				DescriptorSetLayout::Binding{
					.name = "Colors",
					.binding = 1,
					.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.stages = VK_SHADER_STAGE_ALL,
					.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				},
				DescriptorSetLayout::Binding{
					.name = "bsdf_image",
					.binding = 2,
					.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.stages = VK_SHADER_STAGE_ALL,
					.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					.usage = VK_IMAGE_USAGE_STORAGE_BIT,
				},
				DescriptorSetLayout::Binding{
					.name = "Statistics",
					.binding = 3,
					.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.stages = VK_SHADER_STAGE_ALL,
					.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				},
			},
			.binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
		});

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
			.name = name() + ".Set",
			.layout = _set_layout,
			.bindings = {
				Binding{
					.buffer = BufferSegment{.buffer = _ubo->buffer(), .range = Buffer::Range{.begin = 0, .len = ubo_size}},
					.binding = 0,
				},
				Binding{
					.buffer = BufferSegment{.buffer = _ubo->buffer(), .range = Buffer::Range{.begin = ubo_size}},
					.binding = 1,
				},
				Binding{
					.image = _functions_image,
					.binding = 2,
				},
				Binding{
					.buffer = _statistics_buffer,
					.binding = 3,
				},
			},
		});

		_sets_layouts.set(application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::module)].set, _set_layout);


		_render_pass = std::make_shared<RenderPass>(RenderPass::CI{
			.app = application(),
			.name = name() + ".RenderPass",
			.attachments = {
				AttachmentDescription2::MakeFrom(AttachmentDescription2::Flags::Clear, _target),
			},
			.subpasses = {
				SubPassDescription2{
					.colors = {AttachmentReference2{.index = 0}},
				},
			},	
		});

		_framebuffer = std::make_shared<Framebuffer>(Framebuffer::CI{
			.app = application(),
			.name = name() + ".Framebuffer",
			.render_pass = _render_pass,
			.attachments = {
				_target,
			},
		});

		const std::filesystem::path shaders = "ProjectShaders:/";

		_blending = AttachmentBlending::DefaultAlphaBlending();

		if (can_mesh)
		{
			_render_3D_mesh = std::make_shared<MeshCommand>(MeshCommand::CI{
				.app = application(),
				.name = name() + ".Render3D",
				.polygon_mode = &_polygon_mode_3D,
				.cull_mode = VK_CULL_MODE_BACK_BIT,
				.extent = [this](){VkExtent2D a = _sphere_resolution.value(); return VkExtent3D{.width = a.width, .height = a.height, .depth = _num_functions.value()}; },
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.extern_render_pass = _render_pass,
				.color_attachments = {
					GraphicsCommand::ColorAttachment{
						.blending = &_blending,
					}
				},	
				.mesh_shader_path = shaders / "RenderSphericalFunction3D.glsl",
				.fragment_shader_path = shaders / "RenderSphericalFunction3D.glsl",
				.definitions = [this](DefinitionsList& res)
				{
					res.clear();
					if (_polygon_mode_3D == VK_POLYGON_MODE_LINE)
					{
						res.pushBack("BSDF_RENDER_MODE BSDF_RENDER_MODE_WIREFRAME");
					}
					if (_display_in_log2)
					{
						res.pushBack("DISPLAY_IN_LOG2 1");
					}
					res.pushBackFormatted("TARGET_PRIMITIVE {}", _polygon_mode_3D == VK_POLYGON_MODE_FILL ? "TARGET_TRIANGLES" : "TARGET_IMPLICIT_LINES");
				},
			});
		}

		_render_in_vector = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".RenderLines",
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			.draw_count = 1,
			.line_raster_state = GraphicsPipeline::LineRasterizationState{
				.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
			},
			.sets_layouts = _sets_layouts,
			.extern_render_pass = _render_pass,
			.color_attachments = {
				GraphicsCommand::ColorAttachment{
					.blending = &_blending,
				}
			},
			.write_depth = false,
			.depth_compare_op = VK_COMPARE_OP_LESS,
			.vertex_shader_path = shaders / "RenderInputVector.glsl",
			.geometry_shader_path = shaders / "RenderInputVector.glsl",
			.fragment_shader_path = shaders / "RenderInputVector.glsl",
		});


		_render_world_basis = std::make_shared<RenderWorldBasis>(RenderWorldBasis::CI{
			.app = application(),
			.name = name() + ".RenderWorldBasis",
			.sets_layouts = _sets_layouts,
			.extern_render_pass = _render_pass,
			.subpass_index = 0,
			.render_in_log_space = &_display_in_log2,
		});
		

		_compute_statistics = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + "ComputeStatistics",
			.shader_path = shaders / "ComputeStatistics.comp",
			.sets_layouts = _sets_layouts,
			.definitions = [this](DefinitionsList& res)
			{
				res.clear();
			},
		});

		_compute_statistics->pipeline()->setInvalidationCallback(Callback{
			.callback = [this]()
			{
				_generate_statistics |= true;
			},
			.id = this,
		});
	}

	void BSDFViewer::updateResources(UpdateContext& ctx)
	{
		const uint32_t num_functions = _num_functions.value();
		_statistics->mutex.lock();
		_statistics->vector.resize(num_functions);
		_statistics->mutex.unlock();
		const size_t old_size = _colors.size();
		_colors.resize(num_functions);
		const size_t new_size = _colors.size();
		for (size_t i = old_size; i < new_size; ++i)
		{
			const size_t seed = i;
			auto rng = std::mt19937_64(seed);
			std::uniform_real_distribution<float> distrib(0, 1);
			Vector4f color{
				distrib(rng),
				distrib(rng),
				distrib(rng),
				1.0f,
			}; 
			color = Sqrt(color);
			_colors[i] = color;
		}
		

		UBOBase ubo{
			.world_to_proj = _camera->getWorldToProj(),
			.world_to_camera = _camera->getWorldToCam(),
			.direction = Vector3f(std::sin(_inclination), std::cos(_inclination), 0),
			.common_alpha = _common_alpha,
			.reference_function = _reference_function_index,
			.roughness = _roughness,
			.metallic = _metallic,
			.shininess = _shininess,
		};

		const size_t ubo_align = application()->deviceProperties().props2.properties.limits.minUniformBufferOffsetAlignment;
		const size_t ubo_size = std::alignUp(sizeof(UBOBase), ubo_align);

		_ubo->setIFN(0, &ubo, sizeof(UBOBase));
		_ubo->setIFN(ubo_size, _colors.data(), _colors.byte_size());

		_ubo->updateResources(ctx);
		_statistics_buffer->updateResource(ctx);
		_functions_image->updateResource(ctx);
		_set->updateResources(ctx);

		_render_pass->updateResources(ctx);
		ctx.resourcesToUpdateLater() += _framebuffer;

		ctx.resourcesToUpdateLater() += _render_3D_mesh;

		ctx.resourcesToUpdateLater() += _render_in_vector;

		ctx.resourcesToUpdateLater() += _compute_statistics;

		_render_world_basis->updateResources(ctx);
	}

	void BSDFViewer::execute(ExecutionRecorder& exec)
	{
		_ubo->recordTransferIFN(exec);

		exec.bindSet(BindSetInfo{
			.index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::module)].set,
			.set = _set,
		});

		if (_generate_statistics && _num_functions.getCachedValue() > 0)
		{
			exec(application()->getPrebuiltTransferCommands().fill_buffer(FillBuffer::FillInfo{
				.buffer = _statistics_buffer, 
				.value = 0,
			}));
			VkExtent3D extent = VkExtent3D{ .width = _statistics_samples, .height = 1, .depth = _num_functions.getCachedValue(),};
			exec(_compute_statistics->with(ComputeCommand::SingleDispatchInfo{
				.extent = extent,
				.dispatch_threads = true,
			}));
			exec(application()->getPrebuiltTransferCommands().download_buffer(DownloadBuffer::DownloadInfo{
				.src = BufferSegment{.buffer = _statistics_buffer, .range = Buffer::Range{.begin = 0, .len = _statistics->vector.byte_size()}},
				.completion_callback = [stats = _statistics, samples = _statistics_samples](int result, std::shared_ptr<PooledBuffer> const& staging)
				{
					VkResult vk_res = static_cast<VkResult>(result);
					if (staging && vk_res == VK_SUCCESS)
					{
						const void * src = staging->buffer()->map();
						stats->mutex.lock();
						size_t copy_size = std::min(stats->vector.byte_size(), staging->buffer()->createInfo().size);
						std::memcpy(stats->vector.data(), src, copy_size);
						for (size_t i = 0; i < stats->vector.size(); ++i)
						{
							FunctionStatistics & fs = stats->vector[i];
							fs.integral /= float(samples);
							fs.variance_with_reference /= float(samples);
						}
						stats->mutex.unlock();
						staging->buffer()->unMap();
					}
				}
			}));

			_generate_statistics = false;
		}

		std::array<VkClearValue, 1> clear_values = {
			VkClearColorValue{.float32 = {_clear_color.x, _clear_color.y, _clear_color.z, _clear_color.w}},
		};

		exec.beginRenderPass(RenderPassBeginInfo{
			.framebuffer = _framebuffer->instance(),
			.clear_value_count = clear_values.size(),
			.ptr_clear_values = clear_values.data(),
		});

		_render_world_basis->execute(exec, *_camera);
		
		exec(_render_3D_mesh);

		exec(_render_in_vector);

		exec.endRenderPass();

		exec.bindSet(BindSetInfo{
			.index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::module)].set,
			.set = nullptr,
		});
	}

	void BSDFViewer::declareGUI(GuiContext& ctx)
	{
		ImGui::PushID(this);

		ImGui::BeginDisabled(!_num_functions.canSetValue());
		{	
			int n = _num_functions.getCachedValueRef();
			bool changed = ImGui::InputInt("Number of functions", &n);
			if (changed)
			{
				_num_functions.setValue(std::max(n, 0));
				_generate_statistics |= true;
			}
		}
		ImGui::EndDisabled();
		
		static thread_local std::string label;
		_statistics->mutex.lock_shared();
		if (ImGui::BeginListBox("Functions"))
		{
			assert(_colors.size() == _statistics->vector.size());
			for (uint32_t i = 0; i < _colors.size32(); ++i)
			{
				ImGui::PushID(i);
				label = std::format("{}", i);
				Vector4f & color = _colors[i];
				if (ImGui::RadioButton(label.c_str(), _reference_function_index == i))
				{
					_reference_function_index = i;
					_generate_statistics |= true;
				}
				ImGui::SameLine();
				//ImGui::SaveIniSettingsToMemory
				char lbl = 0;
				ImGui::ColorEdit4(&lbl, color.data(), ImGuiColorEditFlags_NoInputs);

				FunctionStatistics const& fs = _statistics->vector[i];
				ImGui::SameLine();
				ImGui::Text("Integral: %.3f, Variance: %.3f", fs.integral, fs.variance_with_reference);
				ImGui::PopID();
			}
		}
		_statistics->mutex.unlock_shared();
		ImGui::EndListBox();

		ImGui::SliderFloat("Common alpha", &_common_alpha, 0, 1, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
		ImGui::Checkbox("Display in log2 scale", &_display_in_log2);
		ImGui::SameLine();
		ImGui::Checkbox("Hemisphere", &_hemisphere);
		if (ImGui::SliderAngle("Inclination", &_inclination, 0, _hemisphere ? 90 : 180, "%.1f deg", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_AlwaysClamp))
		{
			_generate_statistics |= true;
		}

		if (ImGui::SliderFloat("Roughness", &_roughness, 0, 1, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat))
		{
			_generate_statistics |= true;
		}

		if (ImGui::SliderFloat("Metallic", &_metallic, 0, 1, "%.3f", ImGuiSliderFlags_NoRoundToFormat))
		{
			_generate_statistics |= true;
		}

		float log_shininess = std::log2(_shininess);
		if (ImGui::SliderFloat("log2(Shininess)", &log_shininess, 1, 10, "%.3f", ImGuiSliderFlags_NoRoundToFormat))
		{
			_shininess = std::exp2(log_shininess);
			_generate_statistics |= true;
		}

		{
			int resolution = _resolution;
			ImGui::InputInt("Resolution", &resolution, _alignment, 16 * _alignment);
			_resolution = std::max(std::alignUpAssumePo2<uint32_t>(std::max(1, resolution), _alignment), _alignment);
		}

		{
			int log_samples = std::bit_width(_statistics_samples) - 1;
			if (ImGui::InputInt("log2(Statistics Samples)", &log_samples))
			{
				log_samples = std::max(log_samples, 0);
				_statistics_samples = (1 << log_samples);
				_generate_statistics |= true;
			}
			ImGui::BeginDisabled();
			ImGui::InputInt("Statistics Samples", (int*) & _statistics_samples);
			ImGui::EndDisabled();
			if (ImGui::InputInt("Statisitcs Seed", (int*)&_statistics_seed))
			{
				_generate_statistics |= true;
			}
		}


		static thread_local ImGuiListSelection gui_polygon_mode = ImGuiListSelection::CI{
			.name = "Polygon Mode",
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.same_line = true,
			.labels = {"Fill", "Line"},
		};

		gui_polygon_mode.setIndex(static_cast<uint32_t>(_polygon_mode_3D));
		gui_polygon_mode.declare();
		_polygon_mode_3D = static_cast<VkPolygonMode>(gui_polygon_mode.index());

		_render_world_basis->declareGUI(ctx);

		ImGui::Text("Snap camera to:");
		for (char i = 0; i < 3; ++i)
		{
			if (i != 1)
			{
				ImGui::SameLine();
				const char label[2] = {'x' + i, char(0)};
				if (ImGui::Button(label))
				{
					Vector3f axis = Vector3f::Zero();
					axis[i] = -1.0f;
					_camera->position() = axis;
					_camera->direction() = -axis;
					_camera->computeInternal();
				}
			}
		}

		ImGui::PopID();
	}
}