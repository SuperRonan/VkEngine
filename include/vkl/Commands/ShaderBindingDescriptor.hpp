#pragma once

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <array>

#include <vkl/Core/DynamicValue.hpp>
#include <vkl/Core/VulkanCommons.hpp>

#include <vkl/VkObjects/Buffer.hpp>
#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Sampler.hpp>

namespace vkl
{
	class TopLevelAccelerationStructure;
	
	struct CombinedImageSampler
	{
		std::shared_ptr<ImageView> image = {};
		std::shared_ptr<Sampler> sampler = {};
	};

	struct ShaderBindingDescription
	{
		BufferSegment				buffer = {};
		Array<BufferAndRange>		buffers = {};
		std::shared_ptr<ImageView>	image = nullptr;
		Array<std::shared_ptr<ImageView>> images;
		std::shared_ptr<Sampler>	sampler = nullptr;
		Array<std::shared_ptr<Sampler>> samplers = {};
		Array<CombinedImageSampler> combined_images_samplers = {};
		std::shared_ptr<TopLevelAccelerationStructure> tlas = nullptr;
		Array<std::shared_ptr<TopLevelAccelerationStructure>> tlases = {};
		std::string					name = {};
		uint32_t					binding = uint32_t(-1);
	};
	
	using ShaderBindings = std::vector<ShaderBindingDescription>;	
}