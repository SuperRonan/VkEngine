
#include "ImguiCommand.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

namespace vkl
{
	ImguiCommand::ImguiCommand(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_swapchain(ci.swapchain)
	{
		createRenderPass();

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

		_desc_pool = std::make_shared<DescriptorPool>(_app, VK_NULL_HANDLE);
		VkDescriptorPoolCreateInfo desc_ci{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = N,
			.poolSizeCount = (uint32_t)sizes.size(),
			.pPoolSizes = sizes.data(),
		};
		_desc_pool->create(desc_ci);

		ImGui_ImplVulkan_InitInfo ii{
			.Instance = _app->instance(),
			.PhysicalDevice = _app->physicalDevice(),
			.Device = device(),
			.QueueFamily = _app->getQueueFamilyIndices().graphics_family.value(),
			.Queue = _app->queues().graphics,
			.DescriptorPool = *_desc_pool,
			.MinImageCount = static_cast<uint32_t>(2),
			.ImageCount = static_cast<uint32_t>(8),
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		};

		ImGui_ImplVulkan_Init(&ii, *_render_pass);
		
		_swapchain->addInvalidationCallback({
			.callback = [=]() {
				_framebuffers.clear();
			},
			.id = this,
		});

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

	void ImguiCommand::createRenderPass()
	{
		VkAttachmentDescription2 attachement_desc{
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
			.pNext = nullptr,
			.flags = 0,
			.format = _swapchain->format(),
			.samples = VK_SAMPLE_COUNT_1_BIT,// To check
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_GENERAL,
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
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};

		_render_pass = std::make_shared<RenderPass>(RenderPass::CI{
			.app = application(),
			.name = name() + ".RenderPass",
			.attachement_descriptors = { attachement_desc },
			.attachement_ref_per_subpass = { attachement_reference },
			.subpasses = { subpass },
			.dependencies ={ dependency },
			.last_is_depth = false,
		});
	}

	void ImguiCommand::init()
	{
		
	}

	void ImguiCommand::execute(ExecutionContext& context, ExecutionInfo const& ei)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();
		InputSynchronizationHelper synch(context);

		const size_t index = ei.index;

		std::array<Resource, 1> resources = {
			Resource{
				._image = _swapchain->instance()->views()[index],
				._begin_state = ResourceState2{
					._access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					._layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
					._stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				},
				._end_state = ResourceState2{
					._access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					._layout = VK_IMAGE_LAYOUT_GENERAL,
					._stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			},
		};
		synch.addSynch(resources[0]);

		synch.record();
		synch.NotifyContext();

		const VkExtent2D render_area = extract(*_framebuffers[index]->extent());

		std::vector<VkClearValue> clear_values(_framebuffers[index]->size());
		for (size_t i = 0; i < clear_values.size(); ++i)
		{
			clear_values[i] = VkClearValue{
				.color = VkClearColorValue{.int32{0, 0, 0, 1}},
			};
		}
		
		const VkRenderPassBeginInfo render_begin = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = *_render_pass,
			.framebuffer = *_framebuffers[index]->instance(),
			.renderArea = VkRect2D{.offset = VkOffset2D{0, 0}, .extent = render_area},
			.clearValueCount = (uint32_t)clear_values.size(),
			.pClearValues = clear_values.data(),
		};
		vkCmdBeginRenderPass(*cmd, &render_begin, VK_SUBPASS_CONTENTS_INLINE);
		{
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
		}
		vkCmdEndRenderPass(*cmd);

		context.keppAlive(_desc_pool);
		context.keppAlive(_render_pass);
		context.keppAlive(_framebuffers[index]->instance());
	}

	void ImguiCommand::execute(ExecutionContext& ctx)
	{
		ExecutionInfo ei{
			.index = _index,
		};
		execute(ctx, ei);
	}

	bool ImguiCommand::updateResources()
	{
		bool res = false;

		if (_framebuffers.empty())
		{
			createFramebuffers();
			res = true;
		}

		for (auto& fb : _framebuffers)
		{
			res |= fb->updateResources();
		}

		return res;
	}


	ImguiCommand::~ImguiCommand()
	{
		
	}
}