#pragma once

#include "Resource.hpp"

namespace vkl
{
	Resource MakeResource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image)
	{
		Resource res;
		if (!!buffer)
			res._buffer = buffer;
		else if (!!image)
			res._image = image;
		return res;

	}
}