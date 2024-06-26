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