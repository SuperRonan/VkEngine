#pragma once

#include <vkl/App/VkApplication.hpp>
#include <mutex>

#include <that/stl_ext/pointer.hpp>

namespace vkl
{
	class AbstractInstance : public VkObject
	{
	protected:

		size_t _creation_tick = {};
		std::vector<Callback> _destruction_callbacks = {};

	public:

		template <class StringLike>
		constexpr AbstractInstance(VkApplication* app, StringLike&& name = "", size_t creation_tick = 0) :
			VkObject(app, std::forward<StringLike>(name)),
			_creation_tick(creation_tick)
		{}

		virtual ~AbstractInstance() override
		{}

		void callDestructionCallbacks()
		{
			for (auto& ic : _destruction_callbacks)
			{
				ic.callback();
			}
		}

		void addDestructionCallback(Callback const& ic)
		{
			_destruction_callbacks.push_back(ic);
		}

		void removeDestructionCallbacks(const void * ptr)
		{
			for (auto it = _destruction_callbacks.begin(); it < _destruction_callbacks.end(); ++it)
			{
				if (it->id == ptr)
				{
					it = _destruction_callbacks.erase(it);
				}
			}
		}

		size_t creationTick() const
		{
			return _creation_tick;
		}
	};

	class UpdateContext;

	struct UpdateResourcesResult
	{
		bool holds_instance = false;
		bool invalidated = false;
		bool created = false;
		bool cached = false;
	};

	class AbstractInstanceHolder : public VkObject
	{
	protected:

		std::map<uintptr_t, Callback::Function> _invalidation_callbacks = {};
		mutable std::mutex _mutex;

		Dyn<bool> _hold_instance = {};
		size_t _latest_update_tick = {};
		UpdateResourcesResult _latest_update_res;

		std::shared_ptr<AbstractInstance> _instance = {};

		virtual void updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res);

	public:

		template <class StringLike>
		constexpr AbstractInstanceHolder(VkApplication * app, StringLike && name, Dyn<bool> const& hold_instance) :
			VkObject(app, std::forward<StringLike>(name)),
			_hold_instance(hold_instance)
		{
			hold_instance.valueOr(true);
		}

		virtual ~AbstractInstanceHolder() override;

		void callInvalidationCallbacks();

		void setInvalidationCallback(Callback const& ic);

		void removeInvalidationCallback(const void* id);

		//void removeInvalidationCallbacks(const void* id);

		void checkHoldInstance(UpdateResourcesResult& res);

		// Uptate internale resources
		// This version:
		// - Checks the update tick
		// - calls checkHoldInstance
		// - Check the invalidation
		virtual UpdateResourcesResult updateResources(UpdateContext& ctx);

		AbstractInstance* instance() const
		{
			return _instance.get();
		}

		std::shared_ptr<AbstractInstance> const& instancePtr() const
		{
			return _instance;
		}

		constexpr const Dyn<bool>& holdInstance() const
		{
			return _hold_instance;
		}

		constexpr Dyn<bool>& holdInstance()
		{
			return _hold_instance;
		}

		virtual void destroyInstanceIFN();

		std::mutex& mutex()
		{
			return _mutex;
		}

		std::mutex const& mutex() const
		{
			return _mutex;
		}
	};

	template <std::derived_from<AbstractInstance> Instance>
		// requires std::is_pointer_interconvertible_base_of<AbstractInstance, Instance>::value
	class InstanceHolder : public AbstractInstanceHolder
	{
	protected:

	public:

		using InstanceType = Instance;

		template <that::concepts::StringLike StringLike>
		constexpr InstanceHolder(VkApplication* app, StringLike&& name, Dyn<bool> const& hold_instance) :
			AbstractInstanceHolder(app, std::forward<StringLike>(name), hold_instance)
		{}

		Instance* instance() const
		{
			return static_cast<Instance*>(_instance.get());
		}

		std::shared_ptr<Instance> const& instancePtr() const
		{
			return std::reinterpret_pointer_downcast<Instance>(_instance);
		}
	};
}