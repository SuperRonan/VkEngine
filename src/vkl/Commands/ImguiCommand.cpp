
#include <vkl/Commands/ImguiCommand.hpp>

#include <vkl/VkObjects/VulkanExtensionsSet.hpp>

#include <vkl/Execution/RenderPassBeginInfo.hpp>

#include <vkl/App/ImGuiApp.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

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
		std::shared_ptr<SwapchainInstance> _swapchain;
		std::shared_ptr<DescriptorPool> _desc_pool;
		size_t _index = 0;
		VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;

		std::shared_ptr<Fence> _fence_to_wait;

		virtual void clear() override
		{
			ExecutionNode::clear();

			_render_pass_info.clear();
			_swapchain.reset();
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
				if (app->imguiConfigFlags() & ImGuiConfigFlags_ViewportsEnable)
				{
					ImGui::RenderPlatformWindowsDefault();
				}
			}
		}
	};

	ImguiCommand::ImguiCommand(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_queue(ci.queue),
		_swapchain(ci.swapchain)
	{
		createRenderPassIFP();

		initImGui();
		
		_swapchain->setInvalidationCallback({
			.callback = [this]() {
				_framebuffers.clear();
			},
			.id = this,
		});
	}

	ImguiCommand::~ImguiCommand()
	{
		_swapchain->removeInvalidationCallback(this);
		
		shutdownImGui();
		_desc_pool = nullptr;
		_render_pass = nullptr;
		
	}

	void ImguiCommand::createFramebuffers()
	{
		const size_t n = _swapchain->instance()->images().size();
		_framebuffers.resize(n);
		for (size_t i = 0; i < n; ++i)
		{
			_framebuffers[i] = std::make_shared<Framebuffer>(Framebuffer::CI{
				.app = application(),
				.name = name() + std::string(".Framebuffer ") + std::to_string(i),
				.render_pass = _render_pass,
				.attachments = {_swapchain->instance()->views()[i]},
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
					.format = [this]() { return _swapchain->format().value().format; },
					.samples = VK_SAMPLE_COUNT_1_BIT, // swapchain images appears to be only with 1 sample
				},
			}
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

		auto check_vk_result = [](VkResult res)
		{
			if (res == VK_ERROR_DEVICE_LOST)
			{
				VKL_BREAKPOINT_HANDLE;
				std::unique_lock lock(g_mutex);
				std::cout << "ImGui Failed!" << std::endl;
			}
			VK_CHECK(res, "ImGui Failed!");
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
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
			.UseDynamicRendering = _render_pass ? false : true,
			.PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.pNext = nullptr,
				.viewMask = 0,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &_imgui_init_format,
				.depthAttachmentFormat = VK_FORMAT_UNDEFINED,
				.stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
			},
			.CheckVkResultFn = check_vk_result,
		};
		
		ImGui_ImplVulkan_Init(&ii);
		ImGui_ImplVulkan_CreateFontsTexture();

		_fences_to_wait.resize(ii.ImageCount);
	}

	void ImguiCommand::shutdownImGui()
	{
		std::cout << "Shutdown ImGui Vulkan" << std::endl;
		vkDeviceWaitIdle(device());
		ImGui_ImplVulkan_DestroyFontsTexture();
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

		for (auto& fb : _framebuffers)
		{
			res |= fb->updateResources(ctx);
		}

		if (!_render_pass)
		{

		}

		const VkFormat swapchain_format = _swapchain->instance()->createInfo().imageFormat;
		if (swapchain_format != _imgui_format)
		{
			VkRenderPass vk_render_pass = _render_pass ? _render_pass->instance()->handle() : VK_NULL_HANDLE;
			vkDeviceWaitIdle(device());
			ImGui_ImplVulkan_CreateMainPipeline(vk_render_pass, swapchain_format);
			_imgui_format = swapchain_format;
		}

		return res;
	}
}