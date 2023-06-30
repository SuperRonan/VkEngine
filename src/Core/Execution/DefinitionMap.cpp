#include "DefinitionMap.hpp"
#include <sstream>

namespace vkl
{
	const std::string DefinitionsMap::_s_empty = {};

	void DefinitionsMap::setDefinition(std::string const& key, std::string const& value)
	{
		_definitions[key] = value;
		_collapsed.clear();
	}

	bool DefinitionsMap::hasDefinition(std::string const& key) const
	{
		return _definitions.contains(key);
	}

	const std::string& DefinitionsMap::getDefinition(std::string const& key) const
	{
		if (hasDefinition(key))
		{
			return _definitions.at(key);
		}
		else
		{
			return _s_empty;
		}
	}

	void DefinitionsMap::removeDefinition(std::string const& key)
	{
		_definitions.erase(key);
		_collapsed.clear();
	}

	void DefinitionsMap::update()
	{
		using namespace std::string_literals;
		if (!_definitions.empty())
		{
			if (_collapsed.empty())
			{
				callInvalidationCallbacks();
				std::stringstream ss;
				for (const auto& [k, v] : _definitions)
				{
					_collapsed.emplace_back(k + " "s + v);
				}
			}
		}
	}
}