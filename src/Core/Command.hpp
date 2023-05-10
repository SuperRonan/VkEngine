#pragma once

#include "ExecutionContext.hpp"

namespace vkl
{
	class Command : public VkObject
	{
	protected:

	public:

		template <typename StringLike>
		constexpr Command(VkApplication* app, StringLike&& name = "") :
			VkObject(app, std::forward<StringLike>(name))
		{}

		virtual ~Command() = default;

		virtual void init() {};

		virtual void execute(ExecutionContext & context) = 0;

		virtual void prepareUpdate() {};

		virtual bool updateResources(UpdateContext & ctx) { return false; };

	};
}