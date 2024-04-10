#include <Core/VkObjects/AbstractInstance.hpp>

namespace vkl
{
	void AbstractInstanceHolder::callInvalidationCallbacks()
	{
		std::unique_lock lock(_mutex);
		for (auto& ic : _invalidation_callbacks)
		{
			ic.callback();
		}
	}

	void AbstractInstanceHolder::addInvalidationCallback(Callback const& ic)
	{
		std::unique_lock lock(_mutex);
		assert(ic.callback.operator bool());
		_invalidation_callbacks.push_back(ic);
	}

	void AbstractInstanceHolder::removeInvalidationCallbacks(const VkObject* ptr)
	{
		std::unique_lock lock(_mutex);
		auto it = _invalidation_callbacks.begin();
		while (it != _invalidation_callbacks.end())
		{
			if (it->id == ptr)
			{
				// erase and advance
				it = _invalidation_callbacks.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	bool AbstractInstanceHolder::checkHoldInstance()
	{
		const bool hi = holdInstance().value();
		if (!hi)
		{
			destroyInstanceIFN();
		}
		return hi;
	}
}