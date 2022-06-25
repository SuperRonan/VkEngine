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

	void RenderPass::create(VkRenderPassCreateInfo const& ci)
	{
		assert(_handle == VK_NULL_HANDLE);
		VK_CHECK(vkCreateRenderPass(_app->device(), &ci, nullptr, &_handle), "Failed to create a render pass.");
	}

	void RenderPass::destroy()
	{
		assert(_handle);
		vkDestroyRenderPass(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	VkRenderPassCreateInfo RenderPass::createInfo()
	{
		for (size_t i = 0; i < _subpasses.size(); ++i)
		{
			_subpasses[i].colorAttachmentCount = (uint32_t)_attachement_ref_per_subpass[i].size();
			_subpasses[i].pColorAttachments = _attachement_ref_per_subpass[i].data();
		}

		VkRenderPassCreateInfo res = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = (uint32_t)_attachement_descriptors.size(),
			.pAttachments = _attachement_descriptors.data(),
			.subpassCount = (uint32_t)_subpasses.size(),
			.pSubpasses = _subpasses.data(),
			.dependencyCount = (uint32_t)_dependencies.size(),
			.pDependencies = _dependencies.data(),
		};
		return res;
	}

	RenderPass::RenderPass(
		VkApplication* app,
		std::vector<VkAttachmentDescription> const& attachements_desc,
		std::vector<std::vector<VkAttachmentReference>> const& attachement_ref_per_subpass,
		std::vector<VkSubpassDescription> const& subpasses,
		std::vector<VkSubpassDependency> const& dependencies
	) :
		VkObject(app),
		_attachement_descriptors(attachements_desc),
		_attachement_ref_per_subpass(attachement_ref_per_subpass),
		_subpasses(subpasses),
		_dependencies(dependencies)
	{
		create(createInfo());
	}
}