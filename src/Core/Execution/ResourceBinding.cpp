#include "ResourceBinding.hpp"
#include <utility>

namespace vkl
{
	ResourceBinding::ResourceBinding(ShaderBindingDescription const& desc) :
		_binding(desc.binding),
		//_set(desc.set),
		_name(desc.name)
	{
		if (desc.buffer)
		{
			_resource.buffers = {
				BufferAndRange{
					.buffer = desc.buffer,
					.range = desc.buffer_range,
				},
			};
		}
		else if (desc.buffer_array)
		{
			_resource.buffers = desc.buffer_array;
		}
		else if (desc.view)
		{
			_resource.images = {
				desc.view,
			};
		}
		else if (desc.image_array)
		{
			_resource.images.resize(desc.image_array.size());
			for(size_t i=0; i<desc.image_array.size(); ++i)
			{
				_resource.images[i] = desc.image_array[i].view;
				if (desc.image_array[i].sampler)
				{
					if (_samplers.size() != _resource.images.size())
					{
						_samplers.resize(_resource.images.size());
					}
					_samplers[i] = desc.image_array[i].sampler;
				}
			}
		}
		if (desc.sampler)
		{
			_samplers = {
				desc.sampler,
			};
		}
		if (desc.tlas)
		{
			_tlas = {desc.tlas, };
			_type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		}
	}

	ResourceBinding::ResourceBinding(ShaderBindingDescription && desc) :
		_binding(desc.binding),
		//_set(desc.set),
		_name(std::move(desc.name))
	{
		if (desc.buffer)
		{
			_resource.buffers = {
				BufferAndRange{
					.buffer = std::move(desc.buffer),
					.range = std::move(desc.buffer_range),
				},
			};
		}
		else if (desc.buffer_array)
		{
			_resource.buffers = std::move(desc.buffer_array);
		}
		else if (desc.view)
		{
			_resource.images = {
				std::move(desc.view),
			};
		}
		else if (desc.image_array)
		{
			_resource.images.resize(desc.image_array.size());
			for (size_t i = 0; i < desc.image_array.size(); ++i)
			{
				_resource.images[i] = std::move(desc.image_array[i].view);
				if (desc.image_array[i].sampler)
				{
					if (_samplers.size() != _resource.images.size())
					{
						_samplers.resize(_resource.images.size());
					}
					_samplers[i] = std::move(desc.image_array[i].sampler);
				}
			}
		}
		if (desc.sampler)
		{
			_samplers = {
				std::move(desc.sampler),
			};
		}
		if (desc.tlas)
		{
			_tlas = { std::move(desc.tlas), };
			_type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		}
	}
}