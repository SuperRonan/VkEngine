#pragma once

#include <Core/App/VkApplication.hpp>
#include "ImageView.hpp"
#include "RenderPass.hpp"
#include <memory>
#include <vector>

namespace vkl
{
	class FramebufferInstance : public AbstractInstance
	{
	protected:

		std::vector<std::shared_ptr<ImageViewInstance>> _textures = {};
		std::shared_ptr<ImageViewInstance> _depth_stencil = nullptr;
		std::shared_ptr<RenderPassInstance> _render_pass = nullptr;
		VkFramebuffer _handle = VK_NULL_HANDLE;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<RenderPassInstance> render_pass = nullptr;
			std::vector<std::shared_ptr<ImageViewInstance>> targets = {};
			std::shared_ptr<ImageViewInstance> depth_stencil = nullptr;
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

		constexpr size_t count()const
		{
			return _textures.size();
		}

		std::shared_ptr<ImageViewInstance> const& depthStencil()const
		{
			return _depth_stencil;
		}

		VkExtent3D extent()const
		{
			VkExtent3D res;
			if (!_textures.empty() && _textures[0])
			{
				res = _textures[0]->image()->createInfo().extent;
			}
			else if (_depth_stencil)
			{
				res = _depth_stencil->image()->createInfo().extent;
			}
			return res;
		}
	};

	class Framebuffer : public InstanceHolder<FramebufferInstance>
	{
	protected:

		std::vector<std::shared_ptr<ImageView>> _textures = {};
		// Maybe in a future version of vulkan, depth and stencil can be separate images
		std::shared_ptr<ImageView> _depth_stencil = nullptr;
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
			std::shared_ptr<ImageView> depth_stencil = nullptr;
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