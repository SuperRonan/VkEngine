#pragma once

#include "VkApplication.hpp"
#include "ImageView.hpp"
#include <memory>
#include <vector>

namespace vkl
{
	class Framebuffer : public VkObject
	{
	protected:

		std::vector<std::shared_ptr<ImageView>> _textures;
		std::shared_ptr<VkRenderPass> _render_pass = nullptr;
		VkFramebuffer _handle = VK_NULL_HANDLE;

	public:

		constexpr Framebuffer(VkApplication * app = nullptr):
			VkObject(app)
		{}

		Framebuffer(std::vector<std::shared_ptr<ImageView>>&& textures, std::shared_ptr<VkRenderPass> render_pass);

		Framebuffer(Framebuffer const&) = delete;

		Framebuffer(Framebuffer&& other) noexcept;


		~Framebuffer();

		Framebuffer& operator=(Framebuffer const&) = delete;

		Framebuffer& operator=(Framebuffer&& other) noexcept;

		void destroy();

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

		constexpr VkExtent3D extent()const
		{
			return _textures[0]->image()->extent();
		}
	};
}