#pragma once

#include <vector>
#include <string>
#include "DefinitionMap.hpp"

namespace vkl
{
	class UpdateContext
	{
	protected:

		bool _check_shaders = false;
		const DefinitionsMap& _common_definitions;

	public:

		struct CreateInfo
		{
			bool check_shaders = false;
			const DefinitionsMap& common_definitions;
		};
		using CI = CreateInfo;

		UpdateContext(CreateInfo const& ci) :
			_check_shaders(ci.check_shaders),
			_common_definitions(ci.common_definitions)
		{

		}

		constexpr bool checkShaders() const 
		{ 
			return _check_shaders; 
		}

		constexpr DefinitionsMap const& commonDefinitions()
		{
			return _common_definitions;
		}

	};
}