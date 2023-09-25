#pragma once

#include <vector>
#include <string>
#include "DefinitionMap.hpp"
#include <Core/Commands/ShaderBindingDescriptor.hpp>

namespace vkl
{
	class UpdateContext
	{
	protected:

		bool _check_shaders = false;
		const DefinitionsMap& _common_definitions;


		MountingPoints* _mounting_points = nullptr;


	public:

		struct CreateInfo
		{
			bool check_shaders = false;
			const DefinitionsMap& common_definitions;
			MountingPoints* mounting_points = nullptr;
		};
		using CI = CreateInfo;

		UpdateContext(CreateInfo const& ci) :
			_check_shaders(ci.check_shaders),
			_common_definitions(ci.common_definitions),
			_mounting_points(ci.mounting_points)
		{

		}

		constexpr bool checkShaders() const 
		{ 
			return _check_shaders; 
		}

		constexpr DefinitionsMap const& commonDefinitions() const
		{
			return _common_definitions;
		}

		MountingPoints* mountingPoints()
		{
			return _mounting_points;
		}

		const MountingPoints* mountingPoints() const
		{
			return _mounting_points;
		}

	};
}