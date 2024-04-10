#pragma once

#include <Core/App/VkApplication.hpp>
#include "Program.hpp"
#include <Core/VkObjects/AbstractInstance.hpp>

namespace vkl
{

	class RenderPassInstance : public AbstractInstance
	{
	protected:

		MyVector<VkAttachmentDescription2> _attachement_descriptors;
		MyVector<MyVector<VkAttachmentReference2>> _attachement_ref_per_subpass;
		MyVector<VkSubpassDescription2> _subpasses;
		MyVector<VkSubpassDependency2> _dependencies;

		bool _last_is_depth_stencil = false;
		bool _multiview = false;

		VkRenderPassCreateInfo2 _vk_ci2;

		VkRenderPass _handle = VK_NULL_HANDLE;

		void setVulkanName();

		friend class RenderPass;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			MyVector<VkAttachmentDescription2> attachement_descriptors = {};
			MyVector<MyVector<VkAttachmentReference2>> attachement_ref_per_subpass = {};
			MyVector<VkSubpassDescription2> subpasses = {};
			MyVector<VkSubpassDependency2> dependencies = {};
			bool last_is_depth_stencil = false;
			bool multiview = false;
		};
		using CI = CreateInfo;

		RenderPassInstance(CreateInfo const& ci);

		virtual ~RenderPassInstance() override;

		void makeCreateInfo();

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

		MyVector<AttachmentDescription2> _attachement_descriptors;
		MyVector<MyVector<VkAttachmentReference2>> _attachement_ref_per_subpass;
		MyVector<VkSubpassDescription2> _subpasses;
		MyVector<VkSubpassDependency2> _dependencies;
		bool _multiview;

		bool _last_is_depth_stencil = false;

		void createInstance();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			MyVector<AttachmentDescription2> attachement_descriptors = {};
			MyVector<MyVector<VkAttachmentReference2>> attachement_ref_per_subpass = {};
			MyVector<VkSubpassDescription2> subpasses = {};
			MyVector<VkSubpassDependency2> dependencies = {};
			bool last_is_depth_stencil = false;
			bool multiview = false;
			bool create_on_construct = false;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		RenderPass(CreateInfo const& ci);

		virtual ~RenderPass()override;

		bool updateResources(UpdateContext & ctx);

		constexpr const auto& getAttachementDescriptors2()const
		{
			return _attachement_descriptors;
		}
	};
}