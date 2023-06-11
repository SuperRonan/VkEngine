#pragma once

#include <Core/VulkanCommons.hpp>
#include <unordered_map>

namespace vkl
{
	class DefinitionsMap
	{
	protected:

		static const std::string _s_empty;

		using MapType = std::unordered_map<std::string, std::string>;
		MapType _definitions;

		std::vector<std::string> _collapsed;

		std::vector<InvalidationCallback> _invalidation_callbacks;

	public:

		DefinitionsMap() {};

		void setDefinition(std::string const& key, std::string const& value);

		const std::string& getDefinition(std::string const& key) const;

		void removeDefinition(std::string const& key);

		void update();

		std::vector<std::string> const& collapsed()const
		{
			return _collapsed;
		}

		void callInvalidationCallbacks()
		{
			for (auto& ic : _invalidation_callbacks)
			{
				ic.callback();
			}
		}

		void addInvalidationCallback(InvalidationCallback const& ic)
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
}