#include "Executor.hpp"


namespace vkl
{
	void DefinitionsMap::setDefinition(std::string const& key, std::string const& value)
	{
		_definitions[key] = value;
		_collapsed.clear();
	}

	const std::string& DefinitionsMap::getDefinition(std::string const& key) const
	{
		if (_definitions.contains(key))
		{
			return _definitions.at(key);
		}
		else
		{
			return ""s;
		}
	}

	void DefinitionsMap::update()
	{
		if (_collapsed.empty())
		{
			std::stringstream ss;
			for (const auto& [k, v] : _definitions)
			{
				_collapsed.emplace_back(k + " "s + v);
			}
		}
	}
}