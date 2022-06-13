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
		VkFramebuffer _handle = VK_NULL_HANDLE;

	public:

		constexpr Framebuffer(VkApplication * app = nullptr):
			VkObject(app)
		{}

		Framebuffer(std::vector<std::shared_ptr<ImageView>>&& textures);


		void destroy();
	};
}