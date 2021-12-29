#pragma once

#include "VkApplication.hpp"
#include "ImageView.hpp"
#include <memory>
#include <vector>

namespace vkl
{
	class Framebuffer
	{
	protected:

		std::vector<std::shared_ptr<ImageView>> _textures;

	public:

	
	};
}