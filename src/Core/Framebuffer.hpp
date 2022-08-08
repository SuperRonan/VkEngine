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
		VkRenderPass _render_pass = VK_NULL_HANDLE;
		VkFramebuffer _handle = VK_NULL_HANDLE;

	public:

		constexpr Framebuffer(VkApplication * app = nullptr):
			VkObject(app)
		{}

		Framebuffer(std::vector<std::shared_ptr<ImageView>>&& textures, VkRenderPass render_pass);

		Framebuffer(Framebuffer const&) = delete;

		constexpr Framebuffer(Framebuffer&& other) noexcept :
			VkObject(std::move(other)),
			_textures(std::move(other._textures)),
			_render_pass(other._render_pass),
			_handle(other._handle)
		{
			other._render_pass = VK_NULL_HANDLE;
			other._handle = VK_NULL_HANDLE;
		}


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