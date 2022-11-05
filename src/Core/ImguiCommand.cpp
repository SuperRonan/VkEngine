
#include "ImguiCommand.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

namespace vkl
{
	ImguiCommand::ImguiCommand(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_targets(ci.targets)
	{

	}

	void ImguiCommand::createRenderPass()
	{
		VkAttachmentDescription attachement_desc{
			.flags = 0,
			.format = _targets[0]->format(),
			.samples = _targets[0]->image()->sampleCount(),
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_GENERAL,
		};
		
		std::vector<VkAttachmentReference> attachement_reference = { {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		} };

		VkSubpassDescription subpass = {
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
		VkSubpassDependency dependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};

		_render_pass = std::make_shared<RenderPass>(RenderPass(
			application(), 
			{ attachement_desc }, 
			{ attachement_reference }, 
			{ subpass }, 
			{ dependency }
		));

		_framebuffers.resize(_targets.size());
		for (size_t i = 0; i < _targets.size(); ++i)
		{
			_framebuffers[i] = std::shared_ptr<Framebuffer>(new Framebuffer({ _targets[i] }, _render_pass));
		}
	}

	void ImguiCommand::init()
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

		_desc_pool = std::make_shared<DescriptorPool>(DescriptorPool(_app, VK_NULL_HANDLE));
		VkDescriptorPoolCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = N,
			.poolSizeCount = (uint32_t)sizes.size(),
			.pPoolSizes = sizes.data(),
		};
		_desc_pool->create(ci);

		ImGui_ImplVulkan_InitInfo ii{
			.Instance = _app->instance(),
			.PhysicalDevice = _app->physicalDevice(),
			.Device = device(),
			.QueueFamily = _app->getQueueFamilyIndices().graphics_family.value(),
			.Queue = _app->queues().graphics,
			.DescriptorPool = *_desc_pool,
			.MinImageCount = static_cast<uint32_t>(_targets.size()),
			.ImageCount = static_cast<uint32_t>(_targets.size()),
			.MSAASamples = _targets[0]->image()->sampleCount(),
		};

		ImGui_ImplVulkan_Init(&ii, *_render_pass);
	}

	void ImguiCommand::execute(ExecutionContext& context)
	{
		std::shared_ptr<CommandBuffer> cmd = context.getCommandBuffer();

		const size_t index = _index;

		_resources = {
			Resource{
				._images = {_targets[index]},
				._begin_state = ResourceState{
					._access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					._layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
					._stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				},
				._end_state = ResourceState{
					._access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					._layout = VK_IMAGE_LAYOUT_GENERAL,
					._stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				},
				._image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			},
		};

		recordInputSynchronization(*cmd, context);

		const VkExtent2D render_area = extract(_framebuffers[index]->extent());

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
			.framebuffer = *_framebuffers[index],
			.renderArea = VkRect2D{.offset = VkOffset2D{0, 0}, .extent = render_area},
			.clearValueCount = (uint32_t)clear_values.size(),
			.pClearValues = clear_values.data(),
		};
		vkCmdBeginRenderPass(*cmd, &render_begin, VK_SUBPASS_CONTENTS_INLINE);
		{
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);

		}
		vkCmdEndRenderPass(*cmd);
		
		declareResourcesEndState(context);
	}


	ImguiCommand::~ImguiCommand()
	{
		
	}
}