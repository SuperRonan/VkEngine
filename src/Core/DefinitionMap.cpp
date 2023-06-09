#include "DefinitionMap.hpp"
#include <sstream>

namespace vkl
{
	void DefinitionsMap::setDefinition(std::string const& key, std::string const& value)
	{
		_definitions[key] = value;
		_collapsed.clear();
	}

	const std::string& DefinitionsMap::getDefinition(std::string const& key) const
	{
		using namespace std::string_literals;
		if (_definitions.contains(key))
		{
			return _definitions.at(key);
		}
		else
		{
			return ""s;
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