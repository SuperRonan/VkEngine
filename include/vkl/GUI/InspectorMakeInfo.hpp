#pragma once

#include <memory>

namespace vkl
{
	class VkObject;
	namespace GUI
	{
		class Context;
		struct InspectorMakeInfo
		{
			std::shared_ptr<VkObject> target; // Mandatory
			Context* ctx; // May be optional?
		};
	}
}