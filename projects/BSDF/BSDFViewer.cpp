#include "BSDFViewer.hpp"

#include <vkl/Execution/Executor.hpp>

#include <vkl/IO/ImGuiUtils.hpp>

#include <random>

namespace vkl
{
	struct UBOBase
	{
		Matrix4f world_to_proj;
		Matrix3x4f world_to_camera;
		ubo_vec3 direction;
		float common_alpha;
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

		createInternals();
	}

	BSDFViewer::~BSDFViewer()
	{

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

		_ubo = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + ".UBO",
			.size = sizeof(UBOBase) + 4 * sizeof(vec4),
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
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
			},
			.binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
		});

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
			.name = name() + ".Set",
			.layout = _set_layout,
			.bindings = {
				Binding{
					.buffer = BufferSegment{.buffer = _ubo->buffer(), .range = Buffer::Range{.begin = 0, .len = sizeof(UBOBase)}},
					.binding = 0,
				},
				Binding{
					.buffer = BufferSegment{.buffer = _ubo->buffer(), .range = Buffer::Range{.begin = sizeof(UBOBase)}},
					.binding = 1,
				},
				Binding{
					.image = _functions_image,
					.binding = 2,
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
			.attachments = {
				_target,
			},
		});

		const std::filesystem::path shaders = application()->mountingPoints()["ProjectShaders"];

		if (can_mesh)
		{
			_render_3D_mesh = std::make_shared<MeshCommand>(MeshCommand::CI{
				.app = application(),
				.name = name() + ".Render3D",
				.polygon_mode = &_polygon_mode_3D,
				.cull_mode = VK_CULL_MODE_NONE,
				.extent = [this](){VkExtent2D a = _sphere_resolution.value(); return VkExtent3D{.width = a.width, .height = a.height, .depth = _num_functions.value()}; },
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.extern_render_pass = _render_pass,
				.mesh_shader_path = shaders / "RenderSphericalFunction3D.glsl",
				.fragment_shader_path = shaders / "RenderSphericalFunction3D.glsl",
			});
		}
	}

	void BSDFViewer::updateResources(UpdateContext& ctx)
	{
		const size_t old_size = _colors.size();
		_colors.resize(_num_functions.value());
		const size_t new_size = _colors.size();
		for (size_t i = old_size; i < new_size; ++i)
		{
			const size_t seed = i;
			auto rng = std::mt19937_64(seed);
			std::uniform_real_distribution<float> distrib(0, 1);
			vec4 color{
				distrib(rng),
				distrib(rng),
				distrib(rng),
				1.0f,
			};
			color = glm::sqrt(color);
			_colors[i] = color;
		}
		

		UBOBase ubo{
			.world_to_proj = _camera->getWorldToProj(),
			.world_to_camera = Matrix3x4f(glm::transpose(_camera->getWorldToCam())),
			.direction = Vector3f(std::sin(_inclination), std::cos(_inclination), 0),
			.common_alpha = _common_alpha,
		};
		_ubo->setIFN(0, &ubo, sizeof(UBOBase));
		_ubo->setIFN(sizeof(UBOBase), _colors.data(), _colors.byte_size());

		_ubo->updateResources(ctx);
		_functions_image->updateResource(ctx);
		_set->updateResources(ctx);

		_render_pass->updateResources(ctx);
		_framebuffer->updateResources(ctx);

		ctx.resourcesToUpdateLater() += _render_3D_mesh;
	}

	void BSDFViewer::execute(ExecutionRecorder& exec)
	{
		_ubo->recordTransferIFN(exec);

		exec.bindSet(BindSetInfo{
			.index = application()->descriptorBindingGlobalOptions().set_bindings[static_cast<uint32_t>(DescriptorSetName::module)].set,
			.set = _set,
		});

		std::array<VkClearValue, 1> clear_values = {
			VkClearColorValue{.float32 = {0.0f, 0.0f, 0.0f, 0.0f}},
		};

		exec.beginRenderPass(RenderPassBeginInfo{
			.framebuffer = _framebuffer->instance(),
			.clear_value_count = clear_values.size(),
			.ptr_clear_values = clear_values.data(),
		}, VK_SUBPASS_CONTENTS_INLINE);

		exec(_render_3D_mesh);

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
				_num_functions.setValue(n);
			}
		}
		ImGui::EndDisabled();

		ImGui::SliderFloat("Common alpha", &_common_alpha, 0, 1, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

		ImGui::Checkbox("Hemisphere", &_hemisphere);
		ImGui::SliderAngle("Inclination", &_inclination, 0, _hemisphere ? 90 : 180, "%.1f deg", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_AlwaysClamp);

		{
			int resolution = _resolution;
			ImGui::InputInt("Resolution", &resolution, _alignment, 16 * _alignment);
			_resolution = std::max(std::alignUpAssumePo2<uint32_t>(resolution, _alignment), _alignment);
		}

		static thread_local ImGuiListSelection gui_polygon_mode = ImGuiListSelection::CI{
			.name = "Polygon Mode",
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.labels = {"Fill", "Line"},
			.same_line = true,
		};

		gui_polygon_mode.setIndex(static_cast<uint32_t>(_polygon_mode_3D));
		gui_polygon_mode.declare();
		_polygon_mode_3D = static_cast<VkPolygonMode>(gui_polygon_mode.index());

		ImGui::PopID();
	}
}