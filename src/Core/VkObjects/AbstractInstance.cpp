#include <Core/VkObjects/AbstractInstance.hpp>

namespace vkl
{
	void AbstractInstanceHolder::callInvalidationCallbacks()
	{
		std::unique_lock lock(_mutex);
		for (auto & [a, ic] : _invalidation_callbacks)
		{
			ic();
		}
	}

	void AbstractInstanceHolder::setInvalidationCallback(Callback const& ic)
	{
		std::unique_lock lock(_mutex);
		assert(ic.callback.operator bool());
		_invalidation_callbacks[reinterpret_cast<uintptr_t>(ic.id)] = ic.callback;
	}

	void AbstractInstanceHolder::removeInvalidationCallback(const void* id)
	{
		std::unique_lock lock(_mutex);
		assert(_invalidation_callbacks.contains(reinterpret_cast<uintptr_t>(id)));
		_invalidation_callbacks.erase(reinterpret_cast<uintptr_t>(id));
	}

	//void AbstractInstanceHolder::removeInvalidationCallbacks(const void* id)
	//{
	//	std::unique_lock lock(_mutex);
	//	auto it = _invalidation_callbacks.begin();
	//	while (it != _invalidation_callbacks.end())
	//	{
	//		if (it->id == id)
	//		{
	//			// erase and advance
	//			it = _invalidation_callbacks.erase(it);
	//		}
	//		else
	//		{
	//			++it;
	//		}
	//	}
	//}

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