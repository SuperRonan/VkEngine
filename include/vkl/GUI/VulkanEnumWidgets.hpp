#pragma once

#include <vkl/GUI/ImGuiUtils.hpp>

#include <vkl/GUI/Context.hpp>

#include <vulkan/vk_enum_string_helper.h>

namespace vkl::GUI
{
	struct VkBitFieldInspectOptions
	{
		
	};

	// Given an Vulkan enum class name (like VkBufferUsageFlags or VkBufferUsageFlags2)
	// Requires bitfield enum names, not bit enum name (requires VkBufferUsageFlags, not VkBufferUsageFlagBits)
	// Returns the len of the prefix of enum values: len("VK_BUFFER_USAGE_" or len("VK_BUFFER_USAGE_2_")
	constexpr uint CountVkEnumPrefixSize(std::string_view enum_name)
	{
		const bool is2 = enum_name.back() == '2';
		uint res = 3; // "VK_"
		const char* it = enum_name.data() + 2; // Skip the initial "Vk"
		
		if (is2)
		{
			res += 2; // "2_"
		}
		return res;
	}

	template <
		class EnumFlagBits,
		class EnumFlags,
		std::convertible_to<std::function<const char*(EnumFlagBits)>> GetBitLabelFn,
		std::invocable<EnumFlags> GetFlagsLabelFn
	>
	bool InspectVkBitFieldEnum(
		const char* label,
		EnumFlags* value,
		EnumFlags allowed_values,
		EnumFlags available_values,
		GetBitLabelFn const& get_bit_label_fn,
		uint prefix_len = 0, // e.g. len("VK_BUFFER_USAGE_")
		VkBitFieldInspectOptions options = {}
	) {
		bool res = false;
		
		ImGui::LabelValue(label, )
		return res;
	}


} // namespace vkl::GUI
