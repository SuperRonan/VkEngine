#include "ResourceBinding.hpp"
#include <utility>

namespace vkl
{
	ResourceBinding::~ResourceBinding()
	{
#if VKL_RESOURCE_BINDING_USE_UNION
		if (hasImage() || hasSampler())
		{
			images_samplers.~MyVector();
		}
		else if (isAS())
		{
			tlases.~MyVector();
		}
		else //if (isBuffer())
		{
			buffers.~MyVector();
		}
#endif
	}

	ResourceBinding::ResourceBinding(ResourceBinding const& other) :
#if VKL_RESOURCE_BINDING_USE_UNION == 0
		buffers(other.buffers),
		images_samplers(other.images_samplers),
		tlases(other.tlases),
#endif
		begin_state(other.begin_state),
		end_state(other.end_state),
		usage(other.usage),
		binding(other.binding),
		resolved_binding(other.resolved_binding),
		name(other.name),
		type(other.type),
		update_range(other.update_range)
	{
#if VKL_RESOURCE_BINDING_USE_UNION
		if (hasImage() || hasSampler())
		{
			images_samplers = other.images_samplers;
		}
		else if (isAS())
		{
			tlases = other.tlases;
		}
		else if (isBuffer())
		{
			buffers = other.buffers;
		}
#endif
	}

	ResourceBinding::ResourceBinding(ResourceBinding && other) noexcept :
#if VKL_RESOURCE_BINDING_USE_UNION == 0
		buffers(std::move(other.buffers)),
		images_samplers(std::move(other.images_samplers)),
		tlases(std::move(other.tlases)),
#endif
		begin_state(other.begin_state),
		end_state(other.end_state),
		usage(other.usage),
		binding(other.binding),
		resolved_binding(other.resolved_binding),
		name(std::move(other.name)),
		type(other.type),
		update_range(other.update_range)
	{
#if VKL_RESOURCE_BINDING_USE_UNION
		if (hasImage() || hasSampler())
		{
			images_samplers = std::move(other.images_samplers);
		}
		else if (isAS())
		{
			tlases = std::move(other.tlases);
		}
		else if (isBuffer())
		{
			buffers = std::move(other.buffers);
		}
#endif
	}

	ResourceBinding& ResourceBinding::operator=(ResourceBinding const& other)
	{
#if VKL_RESOURCE_BINDING_USE_UNION
		if (unionIndex() != other.unionIndex())
		{
			if (hasImage() || hasSampler())
			{
				images_samplers.clear();
				images_samplers.shrink_to_fit();
			}
			else if (isAS())
			{
				tlases.clear();
				tlases.shrink_to_fit();
			}
			else //if (isBuffer())
			{
				buffers.clear();
				buffers.shrink_to_fit();
			}
		}
		type = other.type;
		if (hasImage() || hasSampler())
		{
			images_samplers = other.images_samplers;
		}
		else if (isAS())
		{
			tlases = other.tlases;
		}
		else //if (isBuffer())
		{
			buffers = other.buffers;
		}
#else
		buffers = other.buffers;
		images_samplers = other.images_samplers;
		tlases = other.tlases;
#endif

		begin_state = other.begin_state;
		end_state = other.end_state;
		usage = other.usage;
		binding = other.binding;
		resolved_binding = other.resolved_binding;
		name = other.name;
		update_range = other.update_range;
		return *this;
	}

	void ResourceBinding::swap(ResourceBinding& other) noexcept
	{
#if VKL_RESOURCE_BINDING_USE_UNION
		const uint32_t ui = unionIndex();
		const uint32_t oui = other.unionIndex();
		if (ui == oui)
		{	
			if(ui == 0)	buffers.swap(other.buffers);
			else if(ui == 1) images_samplers.swap(other.images_samplers);
			else if(ui == 2) tlases.swap(other.tlases);
		}
		else
		{
			Array<BufferSegment> tmp_buffers;
			Array<CombinedImageSampler> tmp_images_samplers;
			Array<std::shared_ptr<TLAS>> tmp_tlases;
			if (ui == 0)
			{
				tmp_buffers = std::move(buffers);
				buffers.clear();
				buffers.shrink_to_fit();
			}
			else if (ui == 1)
			{
				tmp_images_samplers = std::move(images_samplers);
				images_samplers.clear();
				images_samplers.shrink_to_fit();
			}
			else if (ui == 2) 
			{
				tmp_tlases = std::move(tlases);
				tlases.clear();
				tlases.shrink_to_fit();
			}

			if (oui == 0)
			{
				buffers = std::move(other.buffers);
				other.buffers.clear();
				other.buffers.shrink_to_fit();
			}
			else if (oui == 1)
			{
				images_samplers = std::move(other.images_samplers);
				other.images_samplers.clear();
				other.images_samplers.shrink_to_fit();
			}
			else if (oui == 2)
			{
				tlases = std::move(other.tlases);
				other.tlases.clear();
				other.tlases.shrink_to_fit();
			}

			if (ui == 0)
			{
				other.buffers = std::move(tmp_buffers);
			}
			else if (ui == 1)
			{
				other.images_samplers = std::move(tmp_images_samplers);
			}
			else if (ui == 2)
			{
				other.tlases = std::move(tmp_tlases);
			}
		}
#else 
		buffers.swap(other.buffers);
		images_samplers.swap(other.images_samplers);
		tlases.swap(other.tlases);
#endif
		
		std::swap(begin_state, other.begin_state);
		end_state.swap(other.end_state);
		std::swap(usage, other.usage);
		std::swap(binding, other.binding);
		std::swap(resolved_binding, other.resolved_binding);
		name.swap(other.name);
		std::swap(type, other.type);
		std::swap(update_range, other.update_range);
	}
	

	ResourceBinding::ResourceBinding(ShaderBindingDescription const& desc) :
		binding(desc.binding),
		//_set(desc.set),
		name(desc.name)
	{
		uint32_t counter = 0;
		if (desc.buffer)
		{
			buffers = {
				desc.buffer,
			};
		}
		else if (desc.buffers)
		{
			buffers = desc.buffers;
		}
		else if (desc.image || desc.sampler)
		{
			images_samplers = {
				CombinedImageSampler{
					.image = desc.image,
					.sampler = desc.sampler,
				}
			};
		}
		else if (desc.images)
		{
			images_samplers.resize(desc.images.size());
			for(size_t i=0; i < images_samplers.size(); ++i)	images_samplers[i].image = desc.images[i];
		}
		else if (desc.samplers)
		{
			images_samplers.resize(desc.samplers.size());
			for (size_t i = 0; i < images_samplers.size(); ++i)	images_samplers[i].sampler = desc.samplers[i];
			type = VK_DESCRIPTOR_TYPE_SAMPLER;
		}
		else if (desc.combined_images_samplers)
		{
			images_samplers = desc.combined_images_samplers;
			type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}
		else if (desc.tlas)
		{
			tlases = {desc.tlas};
			type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		}
		else if (desc.tlases)
		{
			tlases = desc.tlases;
			type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		}
	}

	ResourceBinding::ResourceBinding(ShaderBindingDescription && desc) :
		binding(desc.binding),
		//_set(desc.set),
		name(std::move(desc.name))
	{
		uint32_t counter = 0;
		if (desc.buffer)
		{
			buffers = {
				std::move(desc.buffer),
			};
		}
		else if (desc.buffers)
		{
			buffers = std::move(desc.buffers);
		}
		else if (desc.image || desc.sampler)
		{
			images_samplers = {
				CombinedImageSampler{
					.image = std::move(desc.image),
					.sampler = std::move(desc.sampler),
				}
			};
		}
		else if (desc.images)
		{
			images_samplers.resize(desc.images.size());
			for (size_t i = 0; i < images_samplers.size(); ++i)	images_samplers[i].image = std::move(desc.images[i]);
		}
		else if (desc.samplers)
		{
			images_samplers.resize(desc.samplers.size());
			for (size_t i = 0; i < images_samplers.size(); ++i)	images_samplers[i].sampler = std::move(desc.samplers[i]);
			type = VK_DESCRIPTOR_TYPE_SAMPLER;
		}
		else if (desc.combined_images_samplers)
		{
			images_samplers = std::move(desc.combined_images_samplers);
			type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}
		else if (desc.tlas)
		{
			tlases = { std::move(desc.tlas) };
			type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		}
		else if (desc.tlases)
		{
			tlases = std::move(desc.tlases);
			type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		}
	}

	void ResourceBinding::removeCallback(VkObject* id)
	{
		if (isBuffer())
		{
			for (size_t i = 0; i < buffers.size(); ++i)
			{
				if (buffers[i])
				{
					buffers[i].buffer->removeInvalidationCallbacks(id);
				}
			}
		}
		else if (hasImage() || hasSampler())
		{
			for (size_t i = 0; i < images_samplers.size(); ++i)
			{
				if (images_samplers[i].image)
				{
					images_samplers[i].image->removeInvalidationCallbacks(id);
				}
				if (images_samplers[i].sampler)
				{
					images_samplers[i].sampler->removeInvalidationCallbacks(id);
				}
			}
		}
		else if (isAS())
		{
			for (size_t i = 0; i < tlases.size(); ++i)
			{
				if (tlases[i])
				{
					tlases[i]->removeInvalidationCallbacks(id);
				}
			}
		}
		else
		{
			NOT_YET_IMPLEMENTED;
		}
	}
}