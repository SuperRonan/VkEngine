#pragma once

#include <that/core/Concepts.hpp>
#include <vkl/Core/VulkanCommons.hpp>

namespace vku
{
	template <that::concepts::Enumeration Enum>
	struct EnumMetaInfo
	{
		static constinit const bool IsBitField = false;
		static constinit const unsigned int UniqueId = unsigned(-1);
		using EnumType = Enum;
		static constexpr const char* const Name = nullptr;
		static constexpr const char* const ValuePrefix = nullptr;
		static constexpr const char* GetValueName(Enum value) {
			return nullptr;
		}
	};
}
