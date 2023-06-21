#pragma once

#include <memory>
#include <string>
#include <vector>
#include <Core/DynamicValue.hpp>

namespace vkl
{
	class Buffer;
	class ImageView;
	class Sampler;

	struct ShaderBindingDescription
	{
		std::shared_ptr<Buffer>		buffer = nullptr;
		DynamicValue<Range_st>		buffer_range = Range_st{0, 0};
		std::shared_ptr<ImageView>	view = nullptr;
		std::shared_ptr<Sampler>	sampler = nullptr;
		std::string					name = {};
		uint32_t					set = uint32_t(0);
		uint32_t					binding = uint32_t(-1);
	};
	
	using ShaderBindings = std::vector<ShaderBindingDescription>;
	using Bindings = ShaderBindings;
}