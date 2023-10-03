#pragma once

#include <memory>
#include <vector>

namespace vkl
{
	template <class Value>
	class GenericCache
	{
		public:

		virtual void clear() = 0;

	};

	template <class Key, class Value>
	class GenericCacheImpl : public GenericCache<Value>
	{
	protected:
		
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
			_values.clear();
		}

		std::shared_ptr<Value> findIFP(Key const& key) const
		{
			for (size_t i = 0; i < _values.size(); ++i)
			{
				if (_values[i].key == key)
				{
					return _values[i].value;
				}
			}
			return nullptr;
		}

		void recordValue(Key&& k, std::shared_ptr<Value>&& v)
		{
			assert(findIFP(k) == nullptr);
			_values.emplace_back(CachedValue{
				.key = std::move(k),
				.value = std::move(v),
			});
		}

		void recordValue(Key const& k, std::shared_ptr<Value> const& v)
		{
			assert(findIFP(k) == nullptr);
			_values.emplace_back(CachedValue{
				.key = k,
				.value = v,
			});
		}
	};
}