#include "RenderPass.hpp"
#include <cassert>

namespace vkl
{
	RenderPassInstance::~RenderPassInstance()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void RenderPassInstance::setVulkanName()
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
			_app->nameVkObjectIFP(vk_name);
		}
	}

	void RenderPassInstance::create(VkRenderPassCreateInfo2 const& ci)
	{
		assert(_handle == VK_NULL_HANDLE);
		VK_CHECK(vkCreateRenderPass2(_app->device(), &ci, nullptr, &_handle), "Failed to create a render pass.");
		setVulkanName();
	}

	void RenderPassInstance::destroy()
	{
		assert(_handle);
		vkDestroyRenderPass(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	VkRenderPassCreateInfo2 RenderPassInstance::createInfo2()
	{
		for (size_t i = 0; i < _subpasses.size(); ++i)
		{
			_subpasses[i].pColorAttachments = _attachement_ref_per_subpass[i].data();
			if (_last_is_depth_stencil)
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

	RenderPassInstance::RenderPassInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_attachement_descriptors(ci.attachement_descriptors),
		_attachement_ref_per_subpass(ci.attachement_ref_per_subpass),
		_subpasses(ci.subpasses),
		_dependencies(ci.dependencies),
		_last_is_depth_stencil(ci.last_is_depth_stencil)
	{
		create(createInfo2());
	}











	RenderPass::RenderPass(CreateInfo const& ci) :
		ParentType(ci.app, ci.name),
		_attachement_descriptors(ci.attachement_descriptors),
		_attachement_ref_per_subpass(ci.attachement_ref_per_subpass),
		_subpasses(ci.subpasses),
		_dependencies(ci.dependencies),
		_last_is_depth_stencil(ci.last_is_depth_stencil)
	{
		if (ci.create_on_construct)
		{
			createInstance();
		}
	}

	RenderPass::~RenderPass()
	{
		destroyInstance();
	}

	void RenderPass::createInstance()
	{
		assert(!_inst);

		std::vector<VkAttachmentDescription2> vk_attachements;
		vk_attachements.resize(_attachement_descriptors.size());
		for (size_t i = 0; i < vk_attachements.size(); ++i)
		{
			vk_attachements[i] = *_attachement_descriptors[i];
		}

		_inst = std::make_shared<RenderPassInstance>(RenderPassInstance::CI{
			.app = application(),
			.name = name(),
			.attachement_descriptors = std::move(vk_attachements),
			.attachement_ref_per_subpass = _attachement_ref_per_subpass,
			.subpasses = _subpasses,
			.dependencies = _dependencies,
			.last_is_depth_stencil = _last_is_depth_stencil,
		});
	}

	void RenderPass::destroyInstance()
	{
		callInvalidationCallbacks();
		_inst = nullptr;
	}

	bool RenderPass::updateResources(UpdateContext& ctx)
	{
		bool res = false;

		if (_inst)
		{
			if (_attachement_descriptors.size() == _inst->_attachement_descriptors.size())
			{
				for (size_t i = 0; i < _attachement_descriptors.size(); ++i)
				{
					const VkFormat new_format = _attachement_descriptors[i].format.value();
					if (new_format != _inst->_attachement_descriptors[i].format)
					{
						res = true;
						break;
					}
					const VkSampleCountFlagBits new_samples = _attachement_descriptors[i].samples.value();
					if (new_samples != _inst->_attachement_descriptors[i].samples)
					{
						res = true;
						break;
					}
				}
			}
			else
			{
				res = true;
			}

			if (res)
			{
				destroyInstance();
			}
		}

		if (!_inst)
		{
			res = true;
			createInstance();
		}

		return res;
	}
}