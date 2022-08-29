#pragma once

#include "Executor.hpp"

namespace vkl
{
	class Command : public VkObject
	{
	protected:

	public:

		virtual ~Command() = 0;

		virtual void init() = 0;

		virtual void execute(ExecutionContext & context) = 0;

	};
}