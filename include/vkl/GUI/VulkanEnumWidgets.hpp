#pragma once

#include <vkl/GUI/ImGuiUtils.hpp>

#include <vkl/GUI/Context.hpp>

#include <vkl/VkObjects/VulkanEnumMeta.hpp>

#include <imgui/imgui_internal.h>

namespace vkl::GUI
{
	namespace impl
	{
		static std::string _str_tmp;
	}

	struct VkBitFieldInspectOptions
	{
		
	};

	template <concepts::VkFlagBits EnumFlagsBits>
	bool InspectVkBitField_Detail(
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type* value,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type enabled_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1),
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type available_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1),
		VkBitFieldInspectOptions* options = nullptr
	) {
		using Underlying = typename VkFlagBitsUnderlying<EnumFlagsBits>::type;
		using MetaInfo = ::vku::EnumMetaInfo<EnumFlagsBits>;
		bool res = false;
		const bool all_disabled = enabled_values == Underlying(0);
		const bool show_only_set_bits = all_disabled;
		for (uint32_t i = 0; i < 8 * sizeof(Underlying); ++i)
		{
			Underlying bit = Underlying(1) << Underlying(i);
			bool bit_value = *value & bit;
			if (show_only_set_bits && !bit_value)
			{
				continue;
			}
			const char* bit_label = MetaInfo::GetValueName(EnumFlagsBits(bit));
			if (bit_label)
			{
				bool enable = (enabled_values & bit);
				ImGui::BeginDisabled(!enable);
				res |= ImGui::CheckboxBit(bit_label, *value, i);
				ImGui::EndDisabled();
			}
		}
		return res;
	}

	template <concepts::VkFlagBits EnumFlagsBits>
	bool InspectVkBitField_Detail(
		Context& ctx,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type* value,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type enabled_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1),
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type available_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1)
	) {
		return InspectVkBitField_Detail<EnumFlagsBits>(value, enabled_values, available_values);
	}

	template <concepts::VkFlagBits EnumFlagsBits>
	bool InspectVkBitField(
		const char* label,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type* value,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type enabled_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1),
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type available_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1),
		VkBitFieldInspectOptions * options = nullptr
	) {
		using Underlying = typename VkFlagBitsUnderlying<EnumFlagsBits>::type;
		using MetaInfo = ::vku::EnumMetaInfo<EnumFlagsBits>;
		bool res = false;
		if (!value)
		{
			return res;
		}
		std::string& as_str = impl::_str_tmp;
		as_str = vku::GetFlagsStr<EnumFlagsBits>(*value);
		ImGui::LabelText2(label, as_str.c_str());
		ImGui::SameLine();
		ImGui::PushID(label);
		bool is_open = ImGui::ArrowFlipButton("Detail");
		ImGui::PopID();
		if (is_open)
		{
			InspectVkBitField_Detail<EnumFlagsBits>(value, enabled_values, available_values, options);
			ImGui::Separator();
		}
		return res;
	}

	template <concepts::VkFlagBits EnumFlagsBits>
	bool InspectVkBitField(
		Context& ctx,
		const char* label,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type* value,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type enabled_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1),
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type available_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1)
	) {
		return InspectVkBitField<EnumFlagsBits>(label, value, enabled_values, available_values);
	}

	template <concepts::VkFlagBits EnumFlagsBits>
	void InspectVkBitField(
		const char* label,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type const& value,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type available_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1),
		VkBitFieldInspectOptions* options = nullptr
	) {
		using Underlying = typename VkFlagBitsUnderlying<EnumFlagsBits>::type;
		const Underlying enabled = 0;
		Underlying * p_value = const_cast<Underlying*>(&value);
#if VKL_BUILD_ANY_DEBUG
		const Underlying value_before = value;
#endif
		bool res = InspectVkBitField<EnumFlagsBits>(label, p_value, enabled, available_values, options);
#if VKL_BUILD_ANY_DEBUG
		assert(value_before == value);
#endif
		assert(res == false);
	}

	template <concepts::VkFlagBits EnumFlagsBits>
	void InspectVkBitField(
		Context& ctx,
		const char* label,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type const& value,
		typename VkFlagBitsUnderlying<EnumFlagsBits>::type available_values = typename VkFlagBitsUnderlying<EnumFlagsBits>::type(-1)
	) {
		return InspectVkBitField<EnumFlagsBits>(label, value, available_values);
	}

	template <concepts::VkRawEnum Enum>
	bool InspectVkEnum(const char* label, Enum const& value)
	{
		using MetaInfo = ::vku::EnumMetaInfo<Enum>;
		const char* value_txt = MetaInfo::GetValueName(value);
		if (value_txt)
		{
			ImGui::LabelText2(label, value_txt);
		}
		else
		{
			using Underlying = typename std::underlying_type<Enum>::type;
			static_assert(sizeof(Underlying) == 4);
			const char* fmt = "Unknown Value (%d)";
			ImGui::LabelValue(label, Underlying(value), fmt);
		}
		return false;
	}

	template <concepts::VkRawEnum Enum>
	bool InspectVkEnum(Context& ctx, const char* label, Enum const& value)
	{
		return InspectVkEnum(label, value);
	}

	template <concepts::VkRawEnum Enum>
	struct EnumOption
	{
		Enum value = {};
		bool disabled = false;

		constexpr operator Enum() const
		{
			return value;
		}
	};

	namespace impl
	{
		template <concepts::VkRawEnum Enum, class EnumOptionType = Enum>
		bool InspectVkEnum(Context& ctx, const char* label, Enum* value, std::span<const EnumOptionType> values)
		{
			bool res = false;
			assert(!!value);
			auto get_value_str = [](Enum value) -> const char*
				{
					using MetaInfo = ::vku::EnumMetaInfo<Enum>;
					const char* res = MetaInfo::GetValueName(value);
					if (!res)
					{
						impl::_str_tmp.clear();
						std::format_to(std::back_inserter(impl::_str_tmp), "Unknown Value {}", static_cast<typename std::underlying_type<Enum>::type>(value));
						res = impl::_str_tmp.c_str();
					}
					return res;
				};
			if (ImGui::BeginCombo(label, get_value_str(*value), ImGuiComboFlags_None))
			{
				for (uint32_t i = 0; i < values.size(); ++i)
				{
					Enum vi = values[i];
					bool b = *value == vi;
					constexpr const bool has_disable = std::is_same<typename std::remove_cv<EnumOptionType>::type, EnumOption<Enum>>::value;
					if constexpr (has_disable)
					{
						ImGui::BeginDisabled(values[i].disabled);
					}
					if (ImGui::Selectable(get_value_str(vi), b, ImGuiSelectableFlags_None))
					{
						res = !b;
						*value = vi;
					}
					if (b)
					{
						ImGui::SetItemDefaultFocus();
					}
					if constexpr (has_disable)
					{
						ImGui::EndDisabled();
					}
				}
				ImGui::EndCombo();
			}
			return res;
		}
	}

	template <concepts::VkRawEnum Enum>
	bool InspectVkEnum(Context& ctx, const char* label, Enum* value, std::span<const Enum> values)
	{
		return impl::InspectVkEnum<Enum, Enum>(ctx, label, value, values);
	}

	template <concepts::VkRawEnum Enum>
	bool InspectVkEnum(Context& ctx, const char* label, Enum* value, std::span<const EnumOption<Enum>> values)
	{
		return impl::InspectVkEnum<Enum, EnumOption<Enum>>(ctx, label, value, values);
	}
	
} // namespace vkl::GUI
