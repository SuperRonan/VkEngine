#include "ResourceBinding.hpp"

namespace vkl
{
	ResourceBinding::ResourceBinding(ShaderBindingDescription const& desc) :
		_resource(Resource{
			.buffer = desc.buffer,
			.buffer_range = desc.buffer_range,
			.image_view = desc.view,
		}),
		_binding(desc.binding),
		//_set(desc.set),
		_name(desc.name)
	{
		if (desc.sampler)
			_sampler = desc.sampler;
	}
}