#pragma once

#include <memory>
#include <vector>
#include <shared_mutex>
#include <vkl/Utils/CompileTimeOptional.hpp>

namespace vkl
{
	template <class Value, bool thread_safe = true>
	class GenericCache
	{
	protected:
		
		mutable std::CompileTimeOptional<std::shared_mutex, thread_safe> _mutex;

	public:

		virtual void clear() = 0;

	};

	template <class Key, class Value, bool thread_safe = true>
	class GenericCacheImpl : public GenericCache<Value, thread_safe>
	{
	protected:

		using ParentType = GenericCache<Value, thread_safe>;
		
		struct CachedValue
		{
			Key key;
			std::shared_ptr<Value> value;
		};

		// TODO use a map if keys allow it

		std::vector<CachedValue> _values;

	public:

		virtual void clear() final override
		{
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.lock();
			}
			_values.clear();
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.unlock();
			}
		}

		std::shared_ptr<Value> findIFP(Key const& key) const
		{
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.lock_shared();
			}
			std::shared_ptr<Value> res = nullptr;
			for (size_t i = 0; i < _values.size(); ++i)
			{
				if (_values[i].key == key)
				{
					res = _values[i].value;
					break;
				}
			}
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.unlock_shared();
			}
			return res;
		}

		template <class CreateValueFunction>
		std::shared_ptr<Value> findOrEmplace(Key const& key, CreateValueFunction const& create_value_function)
		{
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.lock();
			}
			std::shared_ptr<Value> res = nullptr;
			{
				for (size_t i = 0; i < _values.size(); ++i)
				{
					if (_values[i].key == key)
					{
						res = _values[i].value;
						break;
					}
				}
				if (!res)
				{
					res = create_value_function();
					_values.emplace_back(CachedValue{
						.key = key,
						.value = res,
					});
				}
			}
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.unlock();
			}
			return res;
		}

		void recordValue(Key&& k, std::shared_ptr<Value>&& v)
		{
			assert(findIFP(k) == nullptr);
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.lock();
			}
			_values.emplace_back(CachedValue{
				.key = std::move(k),
				.value = std::move(v),
			});
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.unlock();
			}
		}

		void recordValue(Key const& k, std::shared_ptr<Value> const& v)
		{
			assert(findIFP(k) == nullptr);
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.lock();
			}
			_values.emplace_back(CachedValue{
				.key = k,
				.value = v,
			});
			if constexpr (thread_safe)
			{
				ParentType::_mutex._.unlock();
			}
		}
	};
}