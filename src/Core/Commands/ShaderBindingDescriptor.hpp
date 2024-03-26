#pragma once

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <array>

#include <Core/DynamicValue.hpp>
#include <Core/VulkanCommons.hpp>

#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Sampler.hpp>

namespace vkl
{
	class TopLevelAccelerationStructure;
	
	struct CombinedImageSampler
	{
		std::shared_ptr<ImageView> view = {};
		std::shared_ptr<Sampler> sampler = {};
	};

	struct ShaderBindingDescription
	{
		std::shared_ptr<Buffer>		buffer = nullptr;
		DynamicValue<Buffer::Range>	buffer_range = Buffer::Range{0, 0};
		Array<BufferAndRange> buffer_array = {};
		std::shared_ptr<ImageView>	view = nullptr;
		std::shared_ptr<Sampler>	sampler = nullptr;
		Array<CombinedImageSampler> image_array = {};
		std::shared_ptr<TopLevelAccelerationStructure> tlas = nullptr;
		std::string					name = {};
		uint32_t					binding = uint32_t(-1);
	};
	
	using ShaderBindings = std::vector<ShaderBindingDescription>;	
}