#pragma once

#include "VkApplication.hpp"
#include "ImageView.hpp"
#include "RenderPass.hpp"
#include <memory>
#include <vector>

namespace vkl
{
	class FramebufferInstance : public VkObject
	{
	protected:

		std::vector<std::shared_ptr<ImageViewInstance>> _textures = {};
		std::shared_ptr<ImageViewInstance> _depth = nullptr;
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		VkFramebuffer _handle = VK_NULL_HANDLE;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<RenderPass> render_pass = nullptr;
			std::vector<std::shared_ptr<ImageViewInstance>> targets = {};
			std::shared_ptr<ImageViewInstance> depth = nullptr;
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

		constexpr const auto& textures()const
		{
			return _textures;
		}

		constexpr auto& textures()
		{
			return _textures;
		}

		constexpr size_t size()const
		{
			return textures().size();
		}

		constexpr size_t layers()const
		{
			return _textures.size();
		}

		VkExtent3D extent()const
		{
			return _textures[0]->image()->createInfo().extent;
		}
	};

	class Framebuffer : public InstanceHolder<FramebufferInstance>
	{
	protected:

		std::vector<std::shared_ptr<ImageView>> _textures = {};
		std::shared_ptr<ImageView> _depth = nullptr;
		std::shared_ptr<RenderPass> _render_pass = nullptr;

		void createInstance();

		void destroyInstance();
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<RenderPass> render_pass = nullptr;
			std::vector<std::shared_ptr<ImageView>> targets = {};
			std::shared_ptr<ImageView> depth = nullptr;
		};

		using CI = CreateInfo;
		
		Framebuffer(CreateInfo const& ci);

		virtual ~Framebuffer() override;

		bool updateResources();

		constexpr auto& renderPass()
		{
			return _render_pass;
		}

		constexpr const auto& renderPass()const
		{
			return _render_pass;
		}

		constexpr const auto& textures()const
		{
			return _textures;
		}

		constexpr auto& textures()
		{
			return _textures;
		}

		constexpr size_t size()const
		{
			return textures().size();
		}

		constexpr size_t layers()const
		{
			return _textures.size();
		}

		DynamicValue<VkExtent3D> extent()const
		{
			return _textures[0]->image()->extent();
		}
	};
}