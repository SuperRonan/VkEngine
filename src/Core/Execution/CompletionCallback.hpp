#pragma once

#include <Core/App/VkApplication.hpp>

namespace vkl
{
	using CompletionCallback = std::function<void(int)>;

	class CallbackHolder : public VkObject
	{
	public:

		int value = 0;

		std::vector<CompletionCallback> callbacks;

		virtual ~CallbackHolder() override
		{
			for (CompletionCallback& cb : callbacks)
			{
				cb(value);
			}
		}
	};
}