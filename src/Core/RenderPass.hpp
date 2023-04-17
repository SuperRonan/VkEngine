#pragma once

#include "VkApplication.hpp"
#include "Program.hpp"

namespace vkl
{
	class RenderPass : public VkObject
	{
	protected:

		std::vector<VkAttachmentDescription> _attachement_descriptors;
		std::vector<std::vector<VkAttachmentReference>> _attachement_ref_per_subpass;
		std::vector<VkSubpassDescription> _subpasses;
		std::vector<VkSubpassDependency> _dependencies;

		bool _last_is_depth = false;

		VkRenderPass _handle = VK_NULL_HANDLE;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<VkAttachmentDescription> attachement_descriptors = {};
			std::vector<std::vector<VkAttachmentReference>> attachement_ref_per_subpass = {};
			std::vector<VkSubpassDescription> subpasses = {};
			std::vector<VkSubpassDependency> dependencies = {};
			bool last_is_depth = false;
		};
		using CI = CreateInfo;

		RenderPass(CreateInfo const& ci);

		virtual ~RenderPass() override;

		VkRenderPassCreateInfo createInfo();

		void create(VkRenderPassCreateInfo const& ci);

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