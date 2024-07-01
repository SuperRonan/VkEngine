#pragma once

#include <Core/Commands/ShaderBindingDescriptor.hpp>
#include <Core/VulkanCommons.hpp>
#include <Core/VkObjects/Sampler.hpp>
#include <Core/VkObjects/TopLevelAccelerationStructure.hpp>

namespace vkl
{
	

	class ResourceBinding
	{
	public:


#ifndef VKL_RESOURCE_BINDING_USE_UNION
#define VKL_RESOURCE_BINDING_USE_UNION 0
#endif

		// Probably should use a std::variant, a bare union is not enough 
		// (type is not always set to identify the union)
		// But is it worth it?
#if VKL_RESOURCE_BINDING_USE_UNION
		union
		{
#endif
		Array<BufferSegment> buffers = {};
		Array<CombinedImageSampler> images_samplers;
		Array<std::shared_ptr<TLAS>> tlases;
#if VKL_RESOURCE_BINDING_USE_UNION
		};
#endif
		
		ResourceState2 begin_state;
		std::optional<ResourceState2> end_state = {};
		VkFlags64 usage = 0;
		
		uint32_t binding = uint32_t(-1);

		uint32_t resolved_binding = uint32_t(-1);
		std::string name = {};
		VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		// .len == 0 -> no update_range
		// .len == -1 -> should update all range
		Range32u update_range = {};

	protected:

		constexpr uint32_t unionIndex()const
		{
			uint32_t res = 0;
			if(hasImage() || hasSampler())	res = 1;
			if(isAS())	res = 2;
			return res;
		}

	public:
		
		ResourceBinding()
		{}

		~ResourceBinding();

		ResourceBinding(ResourceBinding const& other);

		ResourceBinding(ResourceBinding && other) noexcept;

		ResourceBinding& operator=(ResourceBinding const& other);

		ResourceBinding& operator=(ResourceBinding&& other) noexcept
		{
			swap(other);
			return *this;
		}

		ResourceBinding(ShaderBindingDescription const& desc);
		ResourceBinding(ShaderBindingDescription && desc);

		constexpr void resolve(uint32_t b)
		{
			resolved_binding = b;
		}

		constexpr bool isResolved()const
		{
			return resolved_binding != uint32_t(-1);
		}

		constexpr void unResolve()
		{
			resolved_binding = uint32_t(-1);
		}

		constexpr void setBinding(uint32_t b)
		{
			binding = b;
		}

		constexpr bool resolveWithName()const
		{
			return binding == uint32_t(-1);
		}

		constexpr bool isBuffer()const
		{
			return
				type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
				type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		constexpr bool isImage()const
		{
			return
				type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
				type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		}

		constexpr bool isSampler()const
		{
			return type == VK_DESCRIPTOR_TYPE_SAMPLER;
		}

		constexpr bool isCombinedImageSampler() const
		{
			return type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}
		
		constexpr bool hasImage() const
		{
			return isImage() || isCombinedImageSampler();
		}
		
		constexpr bool hasSampler() const
		{
			return isSampler() || isCombinedImageSampler();
		}

		constexpr bool isAS()const
		{
			return type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		}

		constexpr VkDescriptorType vkType()const
		{
			return (VkDescriptorType)type;
		}

		constexpr void release()
		{
			resolved_binding = uint32_t(-1);
		}

		void invalidate(Range32u range = Range32u{.begin = 0, .len = uint32_t(-1)})
		{
			//invalidateAll();
			//return;
			if (update_range.len == 0)
			{
				update_range = range;
			}
			else
			{
				update_range |= range;
			}
		}

		void invalidate(uint32_t i)
		{
			invalidate(Range32u{.begin = i, .len = 1});
		}

		void invalidateAll()
		{
			update_range.begin = 0;
			update_range.len = uint32_t(-1);
		}

		void resetUpdateRange()
		{
			memset(&update_range, 0, sizeof(update_range));
		}

		size_t size()const
		{
			size_t res = 0;
			if (isBuffer())
			{
				res = buffers.size();
			}
			else if (hasImage() || hasSampler())
			{
				res = images_samplers.size();
			}
			else if (isAS())
			{
				res = tlases.size();
			}
			return res;
		}

		template <std::convertible_to<std::function<std::function<void(void)>(uint32_t)>> CallbackGenerator>
		void installCallbacks(CallbackGenerator const& f)
		{
			Callback cb;
			if (isBuffer())
			{
				for (size_t i = 0; i < buffers.size(); ++i)
				{
					if (buffers[i])
					{
						cb.callback = f(i);
						cb.id = buffers.data() + i;
						buffers[i].buffer->setInvalidationCallback(cb);
					}
				}
			}
			else if (hasImage() || hasSampler())
			{
				for (size_t i = 0; i < images_samplers.size(); ++i)
				{
					if (images_samplers[i].image)
					{
						cb.callback = f(i);
						cb.id = images_samplers.data() + i;
						images_samplers[i].image->setInvalidationCallback(cb);
					}
					if (images_samplers[i].sampler)
					{
						cb.callback = f(i);
						cb.id = images_samplers.data() + i;
						images_samplers[i].sampler->setInvalidationCallback(cb);
					}
				}
			}
			else if (isAS())
			{
				for (size_t i = 0; i < tlases.size(); ++i)
				{
					if (tlases[i])
					{
						cb.callback = f(i);
						cb.id = tlases.data() + i;
						tlases[i]->setInvalidationCallback(cb);
					}
				}
			}
			else
			{
				NOT_YET_IMPLEMENTED;
			}
		}

		void installCallback(Callback& cb)
		{
			installCallbacks([&](uint32_t){return cb.callback;});
		}

		void removeCallbacks();

		void resize(size_t size)
		{
			if (isBuffer())
			{
				buffers.resize(size);
			}
			else if (hasImage() || hasSampler())
			{
				images_samplers.resize(size);
			}
			else if (isAS())
			{
				tlases.resize(size);
			}
		}

		void swap(ResourceBinding& other) noexcept;

		std::string_view nameFromResourceIFP() const
		{ 
			std::string_view res;
			if (isBuffer())
			{
				res = buffers.front().buffer->name();
			}
			else if (isImage())
			{
				res = images_samplers.front().image->name();
			}
			else if (isSampler())
			{
				res = images_samplers.front().sampler->name();
			}
			else if (isAS())
			{
				res = tlases.front()->name();
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

namespace std
{
	inline void swap(vkl::ResourceBinding& a, vkl::ResourceBinding& b)
	{
		a.swap(b);
	}
}