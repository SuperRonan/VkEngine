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

		DefinitionsList _collapsed;

		std::vector<Callback> _invalidation_callbacks;

	public:

		DefinitionsMap() {};

		void setDefinition(std::string const& key, std::string const& value);

		bool hasDefinition(std::string const& key) const;

		const std::string& getDefinition(std::string const& key) const;

		void removeDefinition(std::string const& key);

		void update();

		DefinitionsList const& collapsed()const
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

		void setInvalidationCallback(Callback const& ic)
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