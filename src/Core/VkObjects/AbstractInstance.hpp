#pragma once

#include <Core/App/VkApplication.hpp>
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

		void removeDestructionCallbacks(const VkObject* ptr)
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

		MyVector<Callback> _invalidation_callbacks = {};
		mutable std::mutex _mutex;

		Dyn<bool> _hold_instance = true;

	public:

		template <class StringLike>
		constexpr AbstractInstanceHolder(VkApplication * app, StringLike && name, Dyn<bool> hold_instance) :
			VkObject(app, std::forward<StringLike>(name)),
			_hold_instance(hold_instance)
		{}

		virtual ~AbstractInstanceHolder() override = default;

		void callInvalidationCallbacks();

		void addInvalidationCallback(Callback const& ic);

		void removeInvalidationCallbacks(const VkObject* ptr);

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
	};

	template <std::derived_from<AbstractInstance> Instance>
	class InstanceHolder : public AbstractInstanceHolder
	{
	protected:

		SPtr<Instance> _inst = nullptr;
		
	public:

		using InstanceType = Instance;

		template <std::concepts::StringLike StringLike>
		constexpr InstanceHolder(VkApplication* app, StringLike&& name, Dyn<bool> hold_instance) :
			AbstractInstanceHolder(app, std::forward<StringLike>(name), hold_instance)
		{}

		constexpr SPtr<Instance> const& instance()const
		{
			return _inst;
		}

		virtual ~InstanceHolder() override
		{
			if (_invalidation_callbacks)
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