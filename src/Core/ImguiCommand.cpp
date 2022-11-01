
#include "ImguiCommand.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

namespace vkl
{
	ImguiCommand::ImguiCommand(CreateInfo const& ci) :
		DeviceCommand(ci.app, ci.name),
		_target(ci.target)
	{

	}

	void ImguiCommand::init()
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
			.MinImageCount = (uint32_t)1,
			.ImageCount = (uint32_t)1,
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		};

		ImGui_ImplVulkan_Init(&ii, *_render_pass);
	}

	void ImguiCommand::execute(ExecutionContext& context)
	{

	}
}