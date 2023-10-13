#pragma once

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <array>
#include <Core/DynamicValue.hpp>
#include <Core/VulkanCommons.hpp>

namespace vkl
{
	class Buffer;
	class ImageView;
	class Sampler;


	enum class DescriptorSetName : uint32_t
	{
		common = 0,
		scene = 1,
		module = 2,
		shader = 3,
		object = 4,
		MAX_ENUM,
	};

	const static std::string s_descriptor_set_names[] = {
		"common",
		"scene",
		"module",
		"shader",
		"object"
	};

	struct BindingIndex
	{
		uint32_t set = 0;
		uint32_t binding = 0;
		
		constexpr BindingIndex operator+(uint32_t b) const
		{
			return BindingIndex{.set = set, .binding = binding + b};
		}

		std::string asString()const;
	};

	struct ShaderBindingDescription
	{
		std::shared_ptr<Buffer>		buffer = nullptr;
		DynamicValue<Range_st>		buffer_range = Range_st{0, 0};
		std::shared_ptr<ImageView>	view = nullptr;
		std::shared_ptr<Sampler>	sampler = nullptr;
		std::string					name = {};
		//DescriptorSetName			set = DescriptorSetName::MAX_ENUM;
		uint32_t					binding = uint32_t(-1);
	};
	
	using ShaderBindings = std::vector<ShaderBindingDescription>;


	struct DescriptorSetBindingGlobalOptions
	{
		bool use_push_descriptors;
		bool merge_module_and_shader;
		std::vector<BindingIndex> set_bindings;
		uint32_t shader_set;
	};	
}

template<class Stream>
Stream& operator<<(Stream& s, vkl::BindingIndex const& b)
{
	s << "(set = " << b.set << ", binding = " << b.binding << ")";
	return s;
}