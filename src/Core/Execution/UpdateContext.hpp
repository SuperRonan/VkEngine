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

		const ShaderBindings& _common_bindings;


	public:

		struct CreateInfo
		{
			bool check_shaders = false;
			const DefinitionsMap& common_definitions;
			const ShaderBindings& common_bindings;
		};
		using CI = CreateInfo;

		UpdateContext(CreateInfo const& ci) :
			_check_shaders(ci.check_shaders),
			_common_definitions(ci.common_definitions),
			_common_bindings(ci.common_bindings)
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

		constexpr const ShaderBindings & commonShaderBindings()const
		{
			return _common_bindings;
		}

	};
}