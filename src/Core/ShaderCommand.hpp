#pragma once

#include "DeviceCommand.hpp"
#include <utility>
#include <cassert>

namespace vkl
{
	class ResourceBinding
	{
	public:

	protected:
		Resource _resource;
		std::vector<std::shared_ptr<Sampler>> _samplers = {};
		uint32_t _binding = uint32_t(-1);
		uint32_t _set = 0;

		uint32_t _resolved_set = 0, _resolved_binding = uint32_t(-1);
		std::string _name = "";
		VkDescriptorType _type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

	public:

		constexpr void resolve(uint32_t s, uint32_t b)
		{
			_resolved_set = s;
			_resolved_binding = b;
		}

		constexpr bool isResolved()const
		{
			return _resolved_binding != uint32_t(-1);
		}

		constexpr void setBinding(uint32_t s, uint32_t b)
		{
			_set = s;
			_binding = b;
		}

		constexpr bool resolveWithName()const
		{
			return _binding == uint32_t(-1);
		}

		constexpr const std::string& name()const
		{
			return _name;
		}

		template <class StringLike>
		constexpr void setName(StringLike && name)
		{
			_name = std::forward<StringLike>(name);
		}

		constexpr auto type()const
		{
			return _type;
		}

		constexpr void setType(VkDescriptorType type)
		{
			_type = type;
		}

		constexpr bool isBuffer()const
		{
			return
				_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
				_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		constexpr bool isImage()const
		{
			return
				_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
				_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
				_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		constexpr bool isSampler()
		{
			return
				_type == VK_DESCRIPTOR_TYPE_SAMPLER ||
				_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		constexpr auto& buffers()
		{
			return _resource._buffers;
		}

		constexpr auto& images()
		{
			return _resource._images;
		}

		constexpr auto& samplers()
		{
			return _samplers;
		}

		constexpr VkDescriptorType vkType()const
		{
			return (VkDescriptorType)_type;
		}

		constexpr auto set()const
		{
			return _set;
		}

		constexpr auto binding()const
		{
			return _binding;
		}

		constexpr const auto& state()const
		{
			return _resource._state;
		}

		constexpr void setState(ResourceState const& state)
		{
			_resource._state = state;
		}

		constexpr auto& state()
		{
			return _resource._state;
		}

		constexpr const auto& resource()const
		{
			return _resource;
		}

		constexpr auto& resource()
		{
			return _resource;
		}

		constexpr void release()
		{
			_resolved_binding = uint32_t(-1);
		}

	};

	class ShaderCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<Pipeline> _pipeline;
		std::vector<std::shared_ptr<DescriptorPool>> _desc_pools;
		std::vector<std::shared_ptr<DescriptorSet>> _desc_sets;

		std::vector<ResourceBinding> _bindings;

		std::vector<uint8_t> _push_constants_data;


	public:

		virtual ~ShaderCommand() = 0;

		virtual void writeDescriptorSets();

		virtual void resolveBindings();

		virtual void declareDescriptorSetsResources();

		virtual void recordBindings(CommandBuffer& cmd, ExecutionContext& context);

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context) = 0;

		virtual void init() override = 0;

		virtual void execute(ExecutionContext& context) override = 0;

	};
}