#pragma once

#include "VkApplication.hpp"
namespace vkl
{
	class VkObjectWithCallbacks : public VkObject
	{
	protected:

		

		std::vector<InvalidationCallback> _invalidation_callbacks = {};

		

	public:

		template <class StringLike>
		constexpr VkObjectWithCallbacks(VkApplication * app, StringLike && name = "") :
			VkObject(app, std::forward<StringLike>(name))
		{}

		void callInvalidationCallbacks()
		{
			for (auto& ic : _invalidation_callbacks)
			{
				ic.callback();
			}
		}

		void addInvalidationCallback(InvalidationCallback const& ic)
		{
			_invalidation_callbacks.push_back(ic);
		}

		void removeInvalidationCallbacks(const VkObject* ptr)
		{
			for (auto it = _invalidation_callbacks.begin(); it != _invalidation_callbacks.end(); ++it)
			{
				if (it->id == ptr)
				{
					it = _invalidation_callbacks.erase(it);
				}
			}
		}


	};

}