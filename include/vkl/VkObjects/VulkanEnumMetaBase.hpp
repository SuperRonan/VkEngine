#pragma once

#include <vkl/Core/VulkanCommons.hpp>

namespace vkl::concepts
{
	template <class E>
	concept Enumeration = std::is_enum<E>::value;
}

namespace vku
{
	template <::vkl::concepts::Enumeration Enum>
	struct EnumMetaInfo
	{
		static constexpr const bool IsBitField = false;
		static constexpr const unsigned int UniqueId = unsigned(-1);
		using EnumType = Enum;
		static constexpr const char* Name = nullptr;
		static constexpr const char* ValuePrefix = nullptr;
		static constexpr const char* GetValueName(Enum value) {
			return nullptr;
		}
	};
}
