#include <vkl/VkObjects/AbstractInstance.hpp>

#include <vkl/Execution/UpdateContext.hpp>

namespace vkl
{
	void AbstractInstance::RegisterName(VkApplication* app, VkObjectType type, uint64_t handle, const char* name)
	{
		if (name && strlen(name))
		{
			VkDebugUtilsObjectNameInfoEXT buffer_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = type,
				.objectHandle = handle,
				.pObjectName = name,
			};
			app->nameVkObjectIFP(buffer_name);
		}
	}

	AbstractInstanceHolder::AbstractInstanceHolder(std::shared_ptr<AbstractInstance> const& instance):
		VkObject(instance->application(), std::format("{}.StaticDescriptor", instance->name())),
		_latest_update_tick(instance->creationTick()),
		_latest_update_res(UpdateResourcesResult{.holds_instance = true, .invalidated = false, .created = true, .cached = true,}),
		_instance(instance)
	{

	}

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
		const bool invalidation = ctx.invalidationTick() > _latest_update_tick;
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

		if (invalidation)
		{
			res.invalidated = true;
			destroyInstanceIFN();
		}

		updateResourcesInline(ctx, res);

		return res;
	}

	AbstractInstanceHolder::~AbstractInstanceHolder()
	{
		if (!_invalidation_callbacks.empty())
		{
			VKL_BREAKPOINT_HANDLE;
		}
		destroyInstanceIFN();
	}

	void AbstractInstanceHolder::destroyInstanceIFN()
	{
		if (_instance)
		{
			callInvalidationCallbacks();
			_instance.reset();
			_latest_update_res.invalidated = true;
		}
	}
}