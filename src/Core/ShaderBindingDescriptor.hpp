#pragma once

#include <memory>
#include <string>
#include <vector>

namespace vkl
{
	class Buffer;
	class ImageView;
	class Sampler;

	struct ShaderBindingDescription
	{
		std::shared_ptr<Buffer> buffer = nullptr;
		std::shared_ptr<ImageView> view = nullptr;
		std::shared_ptr<Sampler> sampler = nullptr;
		std::string name = {};
		uint32_t set = uint32_t(0);
		uint32_t binding = uint32_t(-1);
	};
	
	using ShaderBindings = std::vector<ShaderBindingDescription>;
	using Bindings = ShaderBindings;
}