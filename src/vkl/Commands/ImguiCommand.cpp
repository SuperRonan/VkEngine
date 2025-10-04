
#include <vkl/Commands/ImguiCommand.hpp>

#include <vkl/VkObjects/VulkanExtensionsSet.hpp>

#include <vkl/Execution/RenderPassBeginInfo.hpp>

#include <vkl/App/ImGuiApp.hpp>

#include <vkl/VkObjects/DetailedVkFormat.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#define IMGUI_IMPL_VULKAN_HAS_SECONDARY_VIEWPORTS_CONTROL 1

namespace vkl
{
	struct ImGuiCommandNode : public ExecutionNode
	{
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		ImGuiCommandNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		RenderPassBeginInfo _render_pass_info = {};
		std::shared_ptr<DescriptorPool> _desc_pool;
		size_t _index = 0;
		VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;

		std::shared_ptr<Fence> _fence_to_wait;

		ColorCorrectionInfo _color_correction;
		ColorCorrectionInfo _viewports_color_correction;

		virtual void clear() override
		{
			ExecutionNode::clear();

			_render_pass_info.clear();
			_desc_pool.reset();
			_index = 0;
			_fence_to_wait.reset();
		}

		virtual void execute(ExecutionContext& ctx)
		{
			if (_fence_to_wait)
			{
				_fence_to_wait->wait();
			}
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();

			ImGui::Render();

			if (_render_pass_info)
			{
				_render_pass_info.recordBegin(ctx);
			}

			vkCmdPushConstants(*cmd, ImGui_ImplVulkan_GetMainPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 4 * sizeof(float), sizeof(ColorCorrectionParams), &_color_correction.params);

			{
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
			}

			if (_render_pass_info)
			{
				_render_pass_info.recordEnd(ctx);
			}

			ctx.keepAlive(_desc_pool);

			AppWithImGui * app = dynamic_cast<AppWithImGui*>(application());
			if (app)
			{
#ifdef IMGUI_HAS_VIEWPORT
				if (app->imguiConfigFlags() & ImGuiConfigFlags_ViewportsEnable)
				{
					ImGui::RenderPlatformWindowsDefault(nullptr, &_viewports_color_correction.params);
				}
#endif
			}
		}
	};

	ImguiCommand::ImguiCommand(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_queue(ci.queue),
		_target_window(ci.target_window)
	{
		createRenderPassIFP();

		initImGui();
		
		_target_window->swapchain()->setInvalidationCallback({
			.callback = [this]() {
				_framebuffers.clear();
			},
			.id = this,
		});

		std::memset(&_color_correction_info, -1, sizeof(ColorCorrectionInfo));
	}

	ImguiCommand::~ImguiCommand()
	{
		if (_custom_frag_shader)
		{
			_custom_frag_shader->removeInvalidationCallback(this);
		}

		_target_window->swapchain()->removeInvalidationCallback(this);
		
		if (_render_pass)
		{
			_render_pass->removeInvalidationCallback(this);
		}

		shutdownImGui();
		_custom_frag_shader = nullptr;
		_desc_pool = nullptr;
		_render_pass = nullptr;
		
	}

	void ImguiCommand::createFramebuffers()
	{
		const size_t n = _target_window->swapchain()->instance()->images().size();
		_framebuffers.resize(n);
		for (size_t i = 0; i < n; ++i)
		{
			_framebuffers[i] = std::make_shared<Framebuffer>(Framebuffer::CI{
				.app = application(),
				.name = name() + std::string(".Framebuffer ") + std::to_string(i),
				.render_pass = _render_pass,
				.attachments = {_target_window->swapchain()->instance()->views()[i]},
			});
		}
	}

	void ImguiCommand::createRenderPassIFP()
	{
		// ImGui require the ext, vk 1.3 is not enough for now (event though the ext has been promoted to core in 1.3)
		const bool can_use_dynamic_rendering = application()->availableFeatures().features_13.dynamicRendering && application()->deviceExtensions().contains(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		_render_pass = std::make_shared<RenderPass>(RenderPass::SPCI{
			.app = application(),
			.name = name() + ".RenderPass",
			.colors = {
				AttachmentDescription2{
					.flags = AttachmentDescription2::Flags::Blend,
					.format = [this]() { return _target_window->swapchain()->format().value().format; },
					.samples = VK_SAMPLE_COUNT_1_BIT, // swapchain images appears to be only with 1 sample
				},
			},
			.create_on_construct = true,
		});

		_render_pass->setInvalidationCallback(Callback{
			.callback = [this](){_re_create_imgui_pipeline |= true;},
			.id = this,
		});
	}

	void ImguiCommand::initImGui()
	{
		const uint32_t N = 1024;
		std::vector<VkDescriptorPoolSize> sizes = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER,					N },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	N },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				N },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				N },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,		N },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,		N },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			N },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			N },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	N },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	N },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,			N },
		};

		_desc_pool = std::make_shared<DescriptorPool>(DescriptorPool::CreateInfoRaw{
			.app = application(),
			.name = name() + ".pool",
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.max_sets = 8,
			.sizes = sizes,
		});

		_custom_frag_shader = std::make_shared<Shader>(Shader::CI{
			.app = application(),
			.name = name() + ".CustomFragShader",
			.source_path = "ShaderLib:/ImGui/frag.slang",
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		});

		_custom_frag_shader->setInvalidationCallback(Callback{
			.callback = [this]() {
				this->_re_create_imgui_pipeline = true;
			},
			.id = this,
		});

		auto check_vk_result = [](VkResult res)
		{
			if (res == VK_ERROR_DEVICE_LOST)
			{
				VKL_BREAKPOINT_HANDLE;
			}
			VK_CHECK(res, "ImGui Failed!");
		};

		_spec_entry = VkSpecializationMapEntry{
			.constantID = 0,
			.offset = 0,
			.size = sizeof(uint32_t),
		};
		_specialization = VkSpecializationInfo{
			.mapEntryCount = 1,
			.pMapEntries = &_spec_entry,
			.dataSize = sizeof(uint32_t),
			.pData = &_color_correction_info.mode,
		};

		_viewports_specialization = VkSpecializationInfo{
			.mapEntryCount = 1,
			.pMapEntries = &_spec_entry,
			.dataSize = sizeof(uint32_t),
			.pData = &_viewports_color_correction_info.mode,
		};

		const float gamma = 2.4f;
#if IMGUI_IMPL_VULKAN_HAS_COLOR_CORRECTION
		const ImGui_ImplVulkan_ColorCorrectionParameters gamma_params = ImGui_ImplVulkan_ColorCorrectionParameters::MakeGamma(gamma, 1);
#endif

		ImGui_ImplVulkan_PipelineInfo main_pipeline_info{
			.RenderPass = _render_pass->instance()->handle(),
			.Subpass = 0,
			.MSAASamples = _render_pass->instance()->getAttachmentDescriptors2().front().samples,
			.PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.pNext = nullptr,
				.viewMask = 0,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &_imgui_init_format,
				.depthAttachmentFormat = VK_FORMAT_UNDEFINED,
				.stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
			},
		};

		ImGui_ImplVulkan_InitInfo ii{
			.Instance = _app->instance(),
			.PhysicalDevice = _app->physicalDevice(),
			.Device = device(),
			.QueueFamily = _queue->familyIndex(),
			.Queue = _queue->handle(),
			.DescriptorPool = *_desc_pool,
			.MinImageCount = static_cast<uint32_t>(2),
			.ImageCount = static_cast<uint32_t>(8), // Why 8? because max swapchain image possible? 
			.PipelineInfoMain = main_pipeline_info,
			.UseDynamicRendering = _render_pass ? false : true,
			.CheckVkResultFn = check_vk_result,
		};

#ifdef IMGUI_HAS_VIEWPORT
		
#endif

		ImGui_ImplVulkan_Init(&ii);

		_fences_to_wait.resize(ii.ImageCount);

		_re_create_imgui_pipeline = true;
	}

	void ImguiCommand::shutdownImGui()
	{
		application()->logger()("Shutdown ImGui Vulkan", Logger::Options::TagInfo);
		application()->deviceWaitIdle();
		ImGui_ImplVulkan_Shutdown();
		_desc_pool = nullptr;
	}

	void ImguiCommand::init()
	{
		
	}
	
	std::shared_ptr<ExecutionNode> ImguiCommand::getExecutionNode(RecordContext& ctx, ExecutionInfo const& ei)
	{
		std::shared_ptr<ImGuiCommandNode> node = _exec_node_cache.getCleanNode<ImGuiCommandNode>([&](){
			return std::make_shared<ImGuiCommandNode>(ImGuiCommandNode::CI{
				.app = application(),
				.name = name(),
			});
		});

		node->setName(name());

		node->_render_pass_info = RenderPassBeginInfo{
			.framebuffer = _framebuffers[ei.index]->instance(),
		};

		node->_desc_pool = _desc_pool;
		node->_index = ei.index;
		
		node->_render_pass_info.exportResources(node->resources(), true);

		node->_color_correction = _color_correction_info;
		node->_viewports_color_correction = _viewports_color_correction_info;
		node->_viewports_color_correction.params.exposure *= _target_window->brightness();

		return node;
	}

	std::shared_ptr<ExecutionNode> ImguiCommand::getExecutionNode(RecordContext& ctx)
	{
		ExecutionInfo ei{
			.index = _index.value(),
		};
		return getExecutionNode(ctx, ei);
	}

	Executable ImguiCommand::with(ExecutionInfo const& ei)
	{
		return [this, ei](RecordContext& ctx)
		{
			return getExecutionNode(ctx, ei);
		};
	}

	bool ImguiCommand::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		if (_render_pass)
		{
			res |= _render_pass->updateResources(ctx);
		}

		if (_render_pass && _framebuffers.empty())
		{
			createFramebuffers();
			res = true;
		}

		if (_custom_frag_shader)
		{
			_custom_frag_shader->updateResources(ctx);
		}

		for (auto& fb : _framebuffers)
		{
			res |= fb->updateResources(ctx);
		}

		if (!_render_pass)
		{

		}

		SwapchainInstance & swapchain = *_target_window->swapchain()->instance();
		const VkSurfaceFormatKHR surface_format = swapchain.format();
		_imgui_init_format = surface_format.format;
		ColorCorrectionInfo cci = _target_window->getColorCorrectionInfo();
		_color_correction_info = cci;



#if IMGUI_IMPL_VULKAN_HAS_COLOR_CORRECTION 
		ImGui_ImplVulkan_ColorCorrectionMethod imgui_method = ImGui_ImplVulkan_ColorCorrection_None;
		ImGui_ImplVulkan_ColorCorrectionParameters imgui_params;

		// Right now, the ImGui palette apears to be in sRGB, thus, we need to reverse it to make it linear
		// This is a hack
		if (cci.mode == ColorCorrectionMode::PassThrough)
		{
			imgui_method = ImGui_ImplVulkan_ColorCorrection_Gamma;
			imgui_params = ImGui_ImplVulkan_ColorCorrectionParameters::MakeGamma(2.4, cci.params.exposure);
		}
		else if (cci.mode == ColorCorrectionMode::sRGB)
		{
			imgui_method = ImGui_ImplVulkan_ColorCorrection_None;
		}
		else if (cci.mode == ColorCorrectionMode::PerceptualQuantization)
		{
			const auto PQ = [](float l)
			{
				const float m1 = 1305.0 / 8192.0;
				const float m2 = 2523.0 / 32.0;
				const float c1 = 107.0 / 128.0;
				const float c2 = 2413.0 / 128.0;
				const float c3 = 2392.0 / 128.0;
				const float Y = l / (10000.0);
				const float Ym1 = std::pow(Y, m1);
				return std::pow((c1 + c2 * Ym1) / (1 + c3 * Ym1), m2);
			};
			const auto FitGammaForPQLinear = [](float w)
			{
				const float alpha = 0.005182;
				const float beta = 3.333;
				return alpha * w + beta;
			};
			const auto FitGammaForPqPow = [](float w)
			{
				const float a = 2.543;
				const float b = 0.1204;
				const float c = 0;
				return a * std::pow(w, b) + c;
			};
			const float fit_gamma = FitGammaForPqPow(cci.params.exposure);
			const float fit_mult = PQ(cci.params.exposure);
			imgui_method = ImGui_ImplVulkan_ColorCorrection_Gamma;
			imgui_params = ImGui_ImplVulkan_ColorCorrectionParameters::MakeGamma(2.4 / fit_gamma, fit_mult);
		}

		ImGui_ImplVulkan_SetMainColorCorrectionParams(imgui_params);

		if (cci.mode != _color_correction_info.mode)
		{
			_color_correction_info.mode = cci.mode;
			_re_create_imgui_pipeline |= true;
		}
#endif
		if(_re_create_imgui_pipeline)
		{
			
			_custom_frag_shader->waitForInstanceCreationIFN();
			VkRenderPass vk_render_pass = _render_pass ? _render_pass->instance()->handle() : VK_NULL_HANDLE;
			application()->deviceWaitIdle();
			ImGui_ImplVulkan_PipelineInfo info{
				.RenderPass = vk_render_pass,
				.Subpass = 0,
				.MSAASamples = _render_pass->instance()->getAttachmentDescriptors2().front().samples,
				.CustomShadersInfo = ImGui_ImplVulkan_CustomShadersInfo{
					.CustomShaderFrag = _custom_frag_shader->instance()->handle(),
					.SpecializationInfoFrag = &_specialization,
					.PushConstantSize = sizeof(ColorCorrectionParams),
					.PushConstantStages = VK_SHADER_STAGE_FRAGMENT_BIT,
				},
			};
			ImGui_ImplVulkan_CreateMainPipeline(&info);
			
#ifdef IMGUI_HAS_VIEWPORT
#if IMGUI_IMPL_VULKAN_HAS_SECONDARY_VIEWPORTS_CONTROL
			VkPresentModeKHR present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;// swapchain.createInfo().presentMode;
			ImGui_ImplVulkan_SecondaryViewportsInfo vp_info{
				.DesiredFormat = surface_format,
				.DesiredPresentMode = present_mode,
				.GetCustomShadersInfo = [](const void* user_data, VkSurfaceFormatKHR format)
				{
					ImguiCommand& that = *reinterpret_cast<ImguiCommand*>(const_cast<void*>(user_data));
					that._viewports_format = format;
					that._viewports_color_correction_info = DeduceColorCorrection(format);
					ImGui_ImplVulkan_CustomShadersInfo res{
						.CustomShaderFrag = that._custom_frag_shader->instance()->handle(),
						.SpecializationInfoFrag = &that._viewports_specialization,
						.PushConstantSize = sizeof(ColorCorrectionParams),
						.PushConstantStages = VK_SHADER_STAGE_FRAGMENT_BIT,
					};
					return res;
				},
				.UserData = this,
			};
			ImGui_ImplVulkan_SetSecondaryViewportsOptions(&vp_info);
#endif
#endif
			_re_create_imgui_pipeline = false;
		}
		else
		{
#ifdef IMGUI_HAS_VIEWPORT

#endif
		}

		return res;
	}
}