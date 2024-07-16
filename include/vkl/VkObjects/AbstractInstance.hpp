#pragma once

#include <vkl/App/VkApplication.hpp>
#include <mutex>

namespace vkl
{
	class AbstractInstance : public VkObject
	{
	protected:

		std::vector<Callback> _destruction_callbacks = {};

	public:

		template <class StringLike>
		constexpr AbstractInstance(VkApplication* app, StringLike&& name = "") :
			VkObject(app, std::forward<StringLike>(name))
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
	};


	class AbstractInstanceHolder : public VkObject
	{
	protected:

		std::map<uintptr_t, std::function<void(void)>> _invalidation_callbacks = {};
		mutable std::mutex _mutex;

		Dyn<bool> _hold_instance = {};

	public:

		template <class StringLike>
		constexpr AbstractInstanceHolder(VkApplication * app, StringLike && name, Dyn<bool> const& hold_instance) :
			VkObject(app, std::forward<StringLike>(name)),
			_hold_instance(hold_instance)
		{
			hold_instance.valueOr(true);
		}

		virtual ~AbstractInstanceHolder() override = default;

		void callInvalidationCallbacks();

		void setInvalidationCallback(Callback const& ic);

		void removeInvalidationCallback(const void* id);

		//void removeInvalidationCallbacks(const void* id);

		bool checkHoldInstance();

		constexpr const Dyn<bool>& holdInstance() const
		{
			return _hold_instance;
		}

		constexpr Dyn<bool>& holdInstance()
		{
			return _hold_instance;
		}

		virtual void destroyInstanceIFN() = 0;

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
	class InstanceHolder : public AbstractInstanceHolder
	{
	protected:

		SPtr<Instance> _inst = nullptr;
		
	public:

		using InstanceType = Instance;

		template <std::concepts::StringLike StringLike>
		constexpr InstanceHolder(VkApplication* app, StringLike&& name, Dyn<bool> const& hold_instance) :
			AbstractInstanceHolder(app, std::forward<StringLike>(name), hold_instance)
		{}

		constexpr SPtr<Instance> const& instance()const
		{
			return _inst;
		}

		virtual ~InstanceHolder() override
		{
			if (!_invalidation_callbacks.empty())
			{
				VKL_BREAKPOINT_HANDLE;
			}
			destroyInstanceIFN();
		}


		virtual void destroyInstanceIFN() override
		{
			if (_inst)
			{
				callInvalidationCallbacks();
				_inst = nullptr;
			}
		}
	};


	
}