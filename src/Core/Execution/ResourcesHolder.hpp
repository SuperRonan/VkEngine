#pragma once

#include "ResourcesLists.hpp"
#include "ResourcesToUpload.hpp"

namespace vkl
{
	
	class ResourcesHolder
	{
	public:

		virtual ResourcesToUpload getResourcesToUpload() {
			return {};
		};

	};
}