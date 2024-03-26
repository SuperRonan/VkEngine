#pragma once

#include <Core/Commands/ShaderBindingDescriptor.hpp>
#include <Core/VulkanCommons.hpp>
#include "Resource.hpp"
#include <Core/VkObjects/Sampler.hpp>
#include <Core/VkObjects/AccelerationStructure.hpp>

namespace vkl
{
	

	class ResourceBinding
	{
	public:

	protected:
		
		Resource _resource = {};
		Array<std::shared_ptr<Sampler>> _samplers = {};
		Array<std::shared_ptr<TLAS>> _tlas = {};
		
		uint32_t _binding = uint32_t(-1);

		uint32_t _resolved_binding = uint32_t(-1);
		std::string _name = "";
		VkDescriptorType _type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		bool _updated = false;

	public:
		
		ResourceBinding()
		{}

		ResourceBinding(ShaderBindingDescription const& desc);
		ResourceBinding(ShaderBindingDescription && desc);

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

		constexpr bool isAS()const
		{
			return _type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		}

		constexpr const auto& tlas()const
		{
			return _tlas;
		}

		constexpr auto& tlas()
		{
			return _tlas;
		}

		constexpr const auto& samplers()const
		{
			return _samplers;
		}

		constexpr auto& samplers()
		{
			return _samplers;
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

		std::string_view nameFromResourceIFP() const
		{
			std::string_view res;
			if (isBuffer() || isImage())
			{
				res = _resource.nameIFP();
			}
			else if (isSampler())
			{
				if (_samplers && _samplers.front())
				{
					res = _samplers.front()->name();
				}
			}
			return res;
		}

		//bool isNull()const
		//{
		//	bool res = false;
		//	if (_type == VK_DESCRIPTOR_TYPE_MAX_ENUM)
		//	{
		//		res = true;
		//	}
		//	else if (isBuffer())
		//	{
		//		res = !_resource.buffer && _resource.buffer_array.empty();
		//	}
		//	else
		//	{
		//		if (isImage())
		//		{
		//			res |= !_resource.image_view && _resource.image_array.empty();
		//		}
		//		if (isSampler())
		//		{
		//			res |= !_sampler && _sampler_array.empty();
		//		}
		//	}
		//	return res;
		//}
	};

	using ResourceBindings = std::vector<ResourceBinding>;
}