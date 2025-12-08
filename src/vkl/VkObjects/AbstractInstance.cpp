#include <vkl/VkObjects/AbstractInstance.hpp>

#include <vkl/Execution/UpdateContext.hpp>

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

	void AbstractInstanceHolder::checkHoldInstance(UpdateResourcesResult& res)
	{
		if (_hold_instance.hasValue())
		{
			const bool prev = _hold_instance.getCachedValue();
			const bool hi = _hold_instance.value();
			if (!prev && hi)
			{
				// When recreating an instance after it was not held for some time, we need to notify dependent objects
				callInvalidationCallbacks();
			}
			else if (!hi)
			{
				res.invalidated = true;
				destroyInstanceIFN();
			}
			res.holds_instance = hi;
		}
		else
		{
			res.holds_instance = true;
		}
	}

	void AbstractInstanceHolder::updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res)
	{
		assertm(false, "Should not be here.");
	}

	UpdateResourcesResult AbstractInstanceHolder::updateResources(UpdateContext& ctx)
	{
		if (_latest_update_tick >= ctx.updateTick())
		{
			auto res = _latest_update_res;
			res.cached = true;
			return res;
		}
		_latest_update_tick = ctx.updateTick();

		auto& res = _latest_update_res = {};
		checkHoldInstance(res);
		if (!res.holds_instance)
		{
			return res;
		}
		updateResourcesInline(ctx, res);

		return res;
	}
}