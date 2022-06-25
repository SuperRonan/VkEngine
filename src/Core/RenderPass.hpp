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

		VkRenderPass _handle = VK_NULL_HANDLE;

	public:

		constexpr RenderPass(VkApplication * app = nullptr, VkRenderPass handle = VK_NULL_HANDLE) noexcept:
			VkObject(app),
			_handle(handle)
		{}

		RenderPass(
			VkApplication * app,
			std::vector<VkAttachmentDescription> const& attachements_desc,
			std::vector<std::vector<VkAttachmentReference>> const& attachement_ref_per_subpass,
			std::vector<VkSubpassDescription> const& subpasses,
			std::vector<VkSubpassDependency> const& dependencies
			);

		RenderPass(RenderPass const&) = delete;

		constexpr RenderPass(RenderPass&& other) noexcept :
			VkObject(std::move(other)),
			_attachement_descriptors(std::move(other._attachement_descriptors)),
			_attachement_ref_per_subpass(std::move(other._attachement_ref_per_subpass)),
			_subpasses(std::move(other._subpasses)),
			_dependencies(std::move(other._dependencies)),
			_handle(other._handle)
		{
			other._handle = VK_NULL_HANDLE;
		}

		RenderPass& operator=(RenderPass const&) = delete;

		constexpr RenderPass& operator=(RenderPass && other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_attachement_descriptors, other._attachement_descriptors);
			std::swap(_attachement_ref_per_subpass, other._attachement_ref_per_subpass);
			std::swap(_subpasses, other._subpasses);
			std::swap(_dependencies, other._dependencies);
			std::swap(_handle, other._handle);
			return *this;
		}

		~RenderPass();

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