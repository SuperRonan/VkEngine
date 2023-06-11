#pragma once

#include <Core/App/VkApplication.hpp>
#include "Program.hpp"

namespace vkl
{
	class RenderPass : public VkObject
	{
	protected:

		std::vector<VkAttachmentDescription2> _attachement_descriptors;
		std::vector<std::vector<VkAttachmentReference2>> _attachement_ref_per_subpass;
		std::vector<VkSubpassDescription2> _subpasses;
		std::vector<VkSubpassDependency2> _dependencies;

		bool _last_is_depth = false;

		VkRenderPass _handle = VK_NULL_HANDLE;

		void setVulkanName();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<VkAttachmentDescription2> attachement_descriptors = {};
			std::vector<std::vector<VkAttachmentReference2>> attachement_ref_per_subpass = {};
			std::vector<VkSubpassDescription2> subpasses = {};
			std::vector<VkSubpassDependency2> dependencies = {};
			bool last_is_depth = false;
		};
		using CI = CreateInfo;

		RenderPass(CreateInfo const& ci);

		virtual ~RenderPass() override;

		VkRenderPassCreateInfo2 createInfo2();

		void create(VkRenderPassCreateInfo2 const& ci);

		void destroy();

		constexpr operator VkRenderPass()const
		{
			return _handle;
		}

		constexpr VkRenderPass handle()const
		{
			return _handle;
		}

	};
}