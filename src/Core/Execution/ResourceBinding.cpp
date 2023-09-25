#include "ResourceBinding.hpp"

namespace vkl
{
	ResourceBinding::ResourceBinding(ShaderBindingDescription const& desc) :
		_resource(MakeResource(desc.buffer, desc.view)),
		_binding(desc.binding),
		//_set(desc.set),
		_name(desc.name)
	{
		if (!desc.buffer_range.hasValue())
		{
			_resource._buffer_range = desc.buffer->fullRange();
		}
		else
		{
			_resource._buffer_range = desc.buffer_range;
		}

		if (!!desc.sampler)
			_sampler = desc.sampler;
	}
}