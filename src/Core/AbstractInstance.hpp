#pragma once

#include "VkApplication.hpp"

namespace vkl
{
	class AbstractInstanceHolder : public VkObject
	{
	protected:

		

		std::vector<InvalidationCallback> _invalidation_callbacks = {};

		

	public:

		template <class StringLike>
		constexpr AbstractInstanceHolder(VkApplication * app, StringLike && name = "") :
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
			for (auto it = _invalidation_callbacks.begin(); it < _invalidation_callbacks.end(); ++it)
			{
				if (it->id == ptr)
				{
					it = _invalidation_callbacks.erase(it);
				}
			}
		}


	};

	template <class Instance>
	class InstanceHolder : public AbstractInstanceHolder
	{
	protected:

		SPtr<Instance> _inst = nullptr;
		
	public:

		template <class StringLike = std::string>
		constexpr InstanceHolder(VkApplication* app, StringLike&& name = {}) :
			AbstractInstanceHolder(app, std::forward<StringLike>(name))
		{}

		constexpr SPtr<Instance> const& instance()const
		{
			return _inst;
		}
	};

}