#pragma once

#include <Core/Commands/ShaderBindingDescriptor.hpp>
#include <Core/VulkanCommons.hpp>
#include "Resource.hpp"
#include <Core/VkObjects/Sampler.hpp>

namespace vkl
{
	

	class ResourceBinding
	{
	public:

	protected:
		Resource _resource = {};
		std::shared_ptr<Sampler> _sampler = {};
		uint32_t _binding = uint32_t(-1);
		//DescriptorSetName _set = DescriptorSetName::MAX_ENUM;

		uint32_t _resolved_binding = uint32_t(-1);
		std::string _name = "";
		VkDescriptorType _type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		bool _updated = false;

	public:
		
		ResourceBinding()
		{}

		ResourceBinding(ShaderBindingDescription const& desc);

		constexpr void resolve(uint32_t b)
		{
			_resolved_binding = b;
		}

		constexpr bool isResolved()const
		{
			return _resolved_binding != uint32_t(-1);
		}

		constexpr void unResolve()
		{
			_resolved_binding = uint32_t(-1);
		}

		constexpr void setBinding(uint32_t b)
		{
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
		constexpr void setName(StringLike&& name)
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

		constexpr bool isSampler()const
		{
			return
				_type == VK_DESCRIPTOR_TYPE_SAMPLER ||
				_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		constexpr auto& buffer()
		{
			return _resource._buffer;
		}

		constexpr auto& image()
		{
			return _resource._image;
		}

		constexpr auto& sampler()
		{
			return _sampler;
		}

		constexpr const auto& sampler()const
		{
			return _sampler;
		}

		constexpr VkDescriptorType vkType()const
		{
			return (VkDescriptorType)_type;
		}

		constexpr auto binding()const
		{
			return _binding;
		}

		constexpr auto resolvedBinding()const
		{
			return _resolved_binding;
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

		constexpr bool updated()const
		{
			return _updated;
		}

		constexpr void setUpdateStatus(bool status)
		{
			_updated = status;
		}

		bool isNull()const
		{
			bool res = false;
			if (_type == VK_DESCRIPTOR_TYPE_MAX_ENUM)
			{
				res = true;
			}
			else if (isBuffer())
			{
				res = !_resource._buffer;
			}
			else
			{
				if (isImage())
				{
					res |= !_resource._image;
				}
				if (isSampler())
				{
					res |= !_sampler;
				}
			}
			return res;
		}
	};

	using ResourceBindings = std::vector<ResourceBinding>;
}