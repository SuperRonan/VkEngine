#include "RenderPass.hpp"
#include <cassert>

namespace vkl
{
	RenderPass::~RenderPass()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void RenderPass::setVulkanName()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT vk_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_RENDER_PASS,
				.objectHandle = (uint64_t)_handle,
				.pObjectName = name().c_str(),
			};
			_app->nameObject(vk_name);
		}
	}

	void RenderPass::create(VkRenderPassCreateInfo2 const& ci)
	{
		assert(_handle == VK_NULL_HANDLE);
		VK_CHECK(vkCreateRenderPass2(_app->device(), &ci, nullptr, &_handle), "Failed to create a render pass.");
		setVulkanName();
	}

	void RenderPass::destroy()
	{
		assert(_handle);
		vkDestroyRenderPass(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	VkRenderPassCreateInfo2 RenderPass::createInfo2()
	{
		for (size_t i = 0; i < _subpasses.size(); ++i)
		{
			_subpasses[i].pColorAttachments = _attachement_ref_per_subpass[i].data();
			if (_last_is_depth)
			{
				_subpasses[i].pDepthStencilAttachment = _attachement_ref_per_subpass[i].data() + _attachement_ref_per_subpass[i].size() - 1;
			}
			else
			{
				// Should be already like this
				_subpasses[i].pDepthStencilAttachment = nullptr;
			}
		}

		VkRenderPassCreateInfo2 res = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = (uint32_t)_attachement_descriptors.size(),
			.pAttachments = _attachement_descriptors.data(),
			.subpassCount = (uint32_t)_subpasses.size(),
			.pSubpasses = _subpasses.data(),
			.dependencyCount = (uint32_t)_dependencies.size(),
			.pDependencies = _dependencies.data(),
			.correlatedViewMaskCount = 0,
			.pCorrelatedViewMasks = nullptr,
		};
		return res;
	}

	RenderPass::RenderPass(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_attachement_descriptors(ci.attachement_descriptors),
		_attachement_ref_per_subpass(ci.attachement_ref_per_subpass),
		_subpasses(ci.subpasses),
		_dependencies(ci.dependencies),
		_last_is_depth(ci.last_is_depth)
	{
		create(createInfo2());
	}
}