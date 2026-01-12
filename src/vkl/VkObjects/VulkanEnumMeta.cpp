#include <vkl/VkObjects/VulkanEnumMeta.hpp>

#include <tuple>

namespace vku
{
	// Could check every VkFlagBits64 enum
	static_assert(std::is_enum<VkAccessFlagBits2>::value, "Make Sure to use a vulkan header with enum 64-bit flags.");

	using VmaEnums = std::tuple<
		VmaAllocatorCreateFlagBits,
		VmaMemoryUsage,
		VmaAllocationCreateFlagBits,
		VmaPoolCreateFlagBits,
		VmaDefragmentationFlagBits,
		VmaDefragmentationMoveOperation,
		VmaVirtualBlockCreateFlagBits,
		VmaVirtualAllocationCreateFlagBits
	>;

	template<::vkl::concepts::Enumeration Enum>
	static consteval bool CheckVkEnumMetaInfo()
	{
		using MetaInfo = EnumMetaInfo<Enum>;
		return MetaInfo::Name != nullptr;
	}

	template<::vkl::concepts::Enumeration ...Enums>
	static consteval bool CheckAllEnumsMetaInfo(std::tuple<Enums...>)
	{
		return (CheckVkEnumMetaInfo<Enums>() && ...);
	}

	static_assert(CheckAllEnumsMetaInfo(VmaEnums{}));
}

namespace vkl
{

}