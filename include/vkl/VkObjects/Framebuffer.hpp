#pragma once

#include <vkl/App/VkApplication.hpp>
#include "ImageView.hpp"
#include "RenderPass.hpp"
#include <memory>
#include <vector>

namespace vkl
{
	class FramebufferInstance : public AbstractInstance
	{
	protected:

		MyVector<std::shared_ptr<ImageViewInstance>> _attachments = {};
		std::shared_ptr<RenderPassInstance> _render_pass = nullptr;
		VkExtent3D _extent = {};

		VkFramebuffer _handle = VK_NULL_HANDLE;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<RenderPassInstance> render_pass = nullptr;
			MyVector<std::shared_ptr<ImageViewInstance>> attachments = {};
			VkExtent3D extent = {};
		};
		using CI = CreateInfo;

		FramebufferInstance(CreateInfo const& ci);

		virtual ~FramebufferInstance() override;


		constexpr operator VkFramebuffer()const
		{
			return _handle;
		}

		constexpr auto handle()const
		{
			return _handle;
		}

		constexpr auto framebuffer()const
		{
			return handle();
		}

		constexpr auto& renderPass()
		{
			return _render_pass;
		}

		constexpr const auto& renderPass()const
		{
			return _render_pass;
		}

		constexpr const auto& attachments()const
		{
			return _attachments;
		}

		constexpr VkExtent3D extent()const
		{
			return _extent;
		}
	};

	class Framebuffer : public InstanceHolder<FramebufferInstance>
	{
	protected:

		MyVector<std::shared_ptr<ImageView>> _attachments = {};
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		Dyn<VkExtent3D> _extent = {};

		void createInstanceIFP();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<RenderPass> render_pass = nullptr;
			MyVector<std::shared_ptr<ImageView>> attachments = {};
			Dyn<VkExtent3D> extent = {};
			Dyn<bool> hold_instance = true;
		};

		using CI = CreateInfo;
		
		Framebuffer(CreateInfo const& ci);

		virtual ~Framebuffer() override;

		bool updateResources(UpdateContext & ctx);

		constexpr auto& renderPass()
		{
			return _render_pass;
		}

		constexpr const auto& renderPass()const
		{
			return _render_pass;
		}

		constexpr const auto& attachments()const
		{
			return _attachments;
		}

		constexpr const DynamicValue<VkExtent3D>& extent()const
		{
			return _extent;
		}

		//constexpr DynamicValue<VkExtent3D>& extent()
		//{
		//	return _extent;
		//}
	};
}