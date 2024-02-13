#pragma once

#include <Core/App/VkApplication.hpp>
#include "Program.hpp"
#include <Core/VkObjects/AbstractInstance.hpp>

namespace vkl
{

	class RenderPassInstance : public AbstractInstance
	{
	protected:

		std::vector<VkAttachmentDescription2> _attachement_descriptors;
		std::vector<std::vector<VkAttachmentReference2>> _attachement_ref_per_subpass;
		std::vector<VkSubpassDescription2> _subpasses;
		std::vector<VkSubpassDependency2> _dependencies;

		bool _last_is_depth_stencil = false;

		VkRenderPass _handle = VK_NULL_HANDLE;

		void setVulkanName();

		friend class RenderPass;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<VkAttachmentDescription2> attachement_descriptors = {};
			std::vector<std::vector<VkAttachmentReference2>> attachement_ref_per_subpass = {};
			std::vector<VkSubpassDescription2> subpasses = {};
			std::vector<VkSubpassDependency2> dependencies = {};
			bool last_is_depth_stencil = false;
		};
		using CI = CreateInfo;

		RenderPassInstance(CreateInfo const& ci);

		virtual ~RenderPassInstance() override;

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

		constexpr const auto& getAttachementDescriptors2()const
		{
			return _attachement_descriptors;
		}

	};

	class RenderPass : public InstanceHolder<RenderPassInstance>
	{
	public:

		struct AttachmentDescription2
		{
			VkStructureType					sType;
			const void*						pNext;
			VkAttachmentDescriptionFlags	flags;
			Dyn<VkFormat>					format;
			Dyn<VkSampleCountFlagBits>		samples;
			VkAttachmentLoadOp				loadOp;
			VkAttachmentStoreOp				storeOp;
			VkAttachmentLoadOp				stencilLoadOp;
			VkAttachmentStoreOp				stencilStoreOp;
			VkImageLayout					initialLayout;
			VkImageLayout					finalLayout;

			VkAttachmentDescription2 operator*()const
			{
				return VkAttachmentDescription2{
					.sType = sType,
					.pNext = pNext,
					.flags = flags,
					.format = *format,
					.samples = *samples,
					.loadOp = loadOp,
					.storeOp = storeOp,
					.stencilLoadOp = stencilLoadOp,
					.stencilStoreOp = stencilStoreOp,
					.initialLayout = initialLayout,
					.finalLayout = finalLayout,
				};
			}
		};
		
	protected:

		using ParentType = InstanceHolder<RenderPassInstance>;

		std::vector<AttachmentDescription2> _attachement_descriptors;
		std::vector<std::vector<VkAttachmentReference2>> _attachement_ref_per_subpass;
		std::vector<VkSubpassDescription2> _subpasses;
		std::vector<VkSubpassDependency2> _dependencies;

		bool _last_is_depth_stencil = false;

		void createInstance();

		void destroyInstance();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<AttachmentDescription2> attachement_descriptors = {};
			std::vector<std::vector<VkAttachmentReference2>> attachement_ref_per_subpass = {};
			std::vector<VkSubpassDescription2> subpasses = {};
			std::vector<VkSubpassDependency2> dependencies = {};
			bool last_is_depth_stencil = false;
			bool create_on_construct = false;
		};
		using CI = CreateInfo;

		RenderPass(CreateInfo const& ci);

		virtual ~RenderPass()override;

		bool updateResources(UpdateContext & ctx);

		void destroyInstanceIFP()
		{
			if (_inst)
			{
				destroyInstance();
			}
		}

		constexpr const auto& getAttachementDescriptors2()const
		{
			return _attachement_descriptors;
		}
	};
}