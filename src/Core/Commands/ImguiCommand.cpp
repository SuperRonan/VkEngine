
#include "ImguiCommand.hpp"

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

		std::shared_ptr<RenderPassInstance> _render_pass;
		std::shared_ptr<FramebufferInstance> _framebuffer;
		std::shared_ptr<ImageViewInstance> _target;
		std::shared_ptr<SwapchainInstance> _swapchain;
		std::shared_ptr<DescriptorPool> _desc_pool;
		size_t _index = 0;

		virtual void clear() override
		{
			ExecutionNode::clear();

			_render_pass.reset();
			_framebuffer.reset();
			_swapchain.reset();
			_target.reset();
			_desc_pool.reset();
			_index = 0;
		}

		virtual void execute(ExecutionContext& ctx)
		{
			std::shared_ptr<CommandBuffer> cmd = ctx.getCommandBuffer();
			ctx.pushDebugLabel(name());

			ImGui::Render();

			const VkExtent2D render_area = extract(_target->image()->createInfo().extent);

			if (_render_pass)
			{
				//std::vector<VkClearValue> clear_values(_framebuffers[index]->size());
				//for (size_t i = 0; i < clear_values.size(); ++i)
				//{
				//	clear_values[i] = VkClearValue{
				//		.color = VkClearColorValue{.int32{0, 0, 0, 1}},
				//	};
				//}

				const VkRenderPassBeginInfo render_begin = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.pNext = nullptr,
					.renderPass = *_render_pass,
					.framebuffer = *_framebuffer,
					.renderArea = VkRect2D{.offset = VkOffset2D{0, 0}, .extent = render_area},
					.clearValueCount = 0,
					.pClearValues = nullptr,
				};
				vkCmdBeginRenderPass(*cmd, &render_begin, VK_SUBPASS_CONTENTS_INLINE);
			}
			else
			{
				VkRenderingAttachmentInfo attachement{
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.pNext = nullptr,
					.imageView = _target->handle(),
					.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.resolveMode = VK_RESOLVE_MODE_NONE,
					.resolveImageView = VK_NULL_HANDLE,
					.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				};
				VkRenderingInfo render_info{
					.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
					.pNext = nullptr,
					.flags = 0,
					.renderArea = VkRect2D{.offset = VkOffset2D{0, 0}, .extent = render_area},
					.layerCount = 1,
					.viewMask = 0,
					.colorAttachmentCount = 1,
					.pColorAttachments = &attachement,
					.pDepthAttachment = nullptr,
					.pStencilAttachment = nullptr,
				};
				vkCmdBeginRendering(*cmd, &render_info);
			}

			{
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
			}

			if (_render_pass)
			{
				vkCmdEndRenderPass(*cmd);
			}
			else
			{
				vkCmdEndRendering(*cmd);
			}

			ctx.keepAlive(_desc_pool);
			if (_render_pass)
			{
				ctx.keepAlive(_render_pass);
				ctx.keepAlive(_framebuffer);
			}
			if(_target)
			{
				ctx.keepAlive(_target);
			}

			ImGui::RenderPlatformWindowsDefault();
			ctx.popDebugLabel();
		}
	};

	ImguiCommand::ImguiCommand(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_swapchain(ci.swapchain)
	{
		createRenderPassIFP();

		initImGui();
		
		_swapchain->addInvalidationCallback({
			.callback = [this]() {
				_framebuffers.clear();
			},
			.id = this,
		});
	}

	ImguiCommand::~ImguiCommand()
	{
		_swapchain->removeInvalidationCallbacks(this);
		
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
				.targets = {_swapchain->instance()->views()[i]},
			});
		}
	}

	void ImguiCommand::createRenderPassIFP()
	{
		// ImGui require the ext, vk 1.3 is not enough for now (event though the ext has been promoted to core in 1.3)
		const bool can_use_dynamic_rendering = application()->availableFeatures().features_13.dynamicRendering && application()->hasDeviceExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		if (false && can_use_dynamic_rendering)
		{

		}
		else
		{
			RenderPass::AttachmentDescription2 attachement_desc{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = 0,
				.format = [this](){return _swapchain->format().value().format;},
				.samples = VK_SAMPLE_COUNT_1_BIT,// To check
				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};
		
			std::vector<VkAttachmentReference2> attachement_reference = { {
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
				.pNext = nullptr,
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			} };

			VkSubpassDescription2 subpass = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = 0,
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.inputAttachmentCount = 0,
				.pInputAttachments = nullptr,
				.colorAttachmentCount = static_cast<uint32_t>(1),
				.pColorAttachments = attachement_reference.data(), // Warning this is dangerous, the data is copied
				.pResolveAttachments = nullptr,
				.pDepthStencilAttachment = nullptr,
				.preserveAttachmentCount = 0,
				.pPreserveAttachments = nullptr,
			};
			VkSubpassDependency2 dependency = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
				.pNext = nullptr,
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = VK_ACCESS_NONE,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
			};

			_render_pass = std::make_shared<RenderPass>(RenderPass::CI{
				.app = application(),
				.name = name() + ".RenderPass",
				.attachement_descriptors = { attachement_desc },
				.attachement_ref_per_subpass = { attachement_reference },
				.subpasses = { subpass },
				.dependencies ={ dependency },
				.last_is_depth_stencil = false,
				.create_on_construct = true,
			});
		}
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

		ImGui_ImplVulkan_InitInfo ii{
			.Instance = _app->instance(),
			.PhysicalDevice = _app->physicalDevice(),
			.Device = device(),
			.QueueFamily = _app->getQueueFamilyIndices().graphics_family.value(),
			.Queue = _app->queues().graphics,
			.DescriptorPool = *_desc_pool,
			.MinImageCount = static_cast<uint32_t>(2),
			.ImageCount = static_cast<uint32_t>(8), // Why 8? because max swapchain image possible? 
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
			.ImageFormat = VK_FORMAT_B8G8R8A8_UNORM,
			.UseDynamicRendering = _render_pass ? false : true,
		};
		
		ImGui_ImplVulkan_Init(&ii);
		ImGui_ImplVulkan_CreateFontsTexture();
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

		node->_render_pass = _render_pass ? _render_pass->instance() : nullptr;
		if (node->_render_pass)
		{
			node->_framebuffer = _framebuffers[ei.index]->instance();
		}

		node->_swapchain = _swapchain->instance();
		{
			node->_target = node->_swapchain->views()[ei.index]->instance();
		}
		node->_desc_pool = _desc_pool;
		node->_index = ei.index;
		

		node->resources() += 
			ImageViewUsage{
				.ivi = node->_target,
				.begin_state = ResourceState2{
					.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
					.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
					.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				},
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			};
		
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