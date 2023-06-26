#pragma once

#include <Core/App/VkApplication.hpp>

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

		std::vector<Callback> _invalidation_callbacks = {};

	public:

		template <class StringLike>
		constexpr AbstractInstanceHolder(VkApplication * app, StringLike && name = "") :
			VkObject(app, std::forward<StringLike>(name))
		{}

		virtual ~AbstractInstanceHolder() override
		{}

		void callInvalidationCallbacks()
		{
			for (auto& ic : _invalidation_callbacks)
			{
				ic.callback();
			}
		}

		void addInvalidationCallback(Callback const& ic)
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

		static_assert(std::is_base_of<AbstractInstance, Instance>::value, "Instance must derive from AbstracInstance");

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

		virtual ~InstanceHolder() override
		{}

	};


	
}