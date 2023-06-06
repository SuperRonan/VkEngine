#pragma once

#include <Core/VkApplication.hpp>

namespace vkl
{
	class Module : public VkObject
	{
	protected:

	public:

		template <class StringLike = std::string>
		Module(VkApplication * app, StringLike && name):
			VkObject(app, std::forward<StringLike>(name))
		{}

		virtual ~Module() override
		{}
	};
}