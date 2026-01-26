#pragma once

#include <vector>
#include <string>
#include "imgui.h"
#include <vkl/Maths/Types.hpp>
#include <vkl/Core/VulkanCommons.hpp>

namespace vkl
{
	class ImGuiListSelection
	{
	public:
		enum class Mode
		{
			RadioButtons,
			Combo,
			Dropdown = Combo,
		};
		
		struct Option
		{
			std::string name = {};
			std::string desc = {};
			bool disable = false;
		};

		struct OptionView
		{
			std::string_view name = {};
			std::string_view desc = {};
			bool disable = false;
		};

	protected:

		std::string _name = {};
		
		Mode _mode = Mode::Combo;
		bool _same_line = false;

		size_t _index = 0;
		MyVector<Option> _options = {};
		
		// 0 : OK no changes
		// 1 : changes, but OK
		// 2 : error
		int validate();

	public:

		struct CreateInfo
		{
			std::string name = {};
			Mode mode = Mode::Combo;
			bool same_line = false;
			// labels xor options
			MyVector<std::string> labels;
			// labels xor options
			MyVector<Option> options;
			size_t default_index = 0;
		};
		using CI = CreateInfo;

		ImGuiListSelection() = default;

		ImGuiListSelection(CreateInfo const& ci);

		ImGuiListSelection& operator=(ImGuiListSelection const&) = default;
		ImGuiListSelection& operator=(ImGuiListSelection&&) = default;
		
		const auto& name()const
		{
			return _name;
		}

		void setOptionsCount(uint32_t count);

		void enableOptions(size_t index, bool enable = true)
		{
			assert(index < _options.size());
			_options[index].disable = !enable;
		}

		void setOption(size_t index, OptionView const& opt);
		void setOption(size_t index, Option && opt);

		void setIndex(size_t index)
		{
			_index = index;
		}

		bool declareRadioButtons(const char* name, size_t & active_index, bool same_line) const;
		
		bool declareRadioButtons(bool same_line)
		{
			return declareRadioButtons(_name.c_str(), _index, same_line);
		}

		bool declareRadioButtons(size_t& active_index, bool same_line) const
		{
			return declareRadioButtons(_name.c_str(), active_index, same_line);
		}

		bool declareRadioButtons(const char* name, bool same_line)
		{
			return declareRadioButtons(name, _index, same_line);
		}
		
		bool declareRadioButtons()
		{
			return declareRadioButtons(_same_line);
		}

		bool declareCombo(const char* name, size_t & active_index) const;
		
		bool declareCombo(const char* name)
		{
			return declareCombo(name, _index);
		}

		bool declareCombo(size_t& active_index) const
		{
			return declareCombo(_name.c_str(), active_index);
		}

		bool declareCombo()
		{
			return declareCombo(_name.c_str(), _index);
		}

		bool declareDropdown(const char* name, size_t& active_index) const
		{
			return declareCombo(name, active_index);
		}

		bool declareDropdown(size_t & active_index) const
		{
			return declareDropdown(_name.c_str(), active_index);
		}

		bool declareDropdown(const char* name)
		{
			return declareDropdown(name, _index);
		}

		bool declareDropdown()
		{
			return declareCombo();
		}

		bool declare(const char* name, size_t & active_index) const
		{
			if (_mode == Mode::RadioButtons)
			{
				return declareRadioButtons(name, active_index, _same_line);
			}
			else if (_mode == Mode::Combo)
			{
				return declareCombo(name, active_index);
			}
			return false;
		}

		bool declare(size_t& active_index) const
		{
			return declare(_name.c_str(), active_index);
		}

		bool declare(const char* name)
		{
			return declare(name, _index);
		}

		bool declare()
		{
			return declare(_index);
		}

		constexpr size_t index()const
		{
			return _index;
		}

		const auto& options()const
		{
			return _options;
		}
	};

	class ImGuiTransform3D
	{
	protected:

		Matrix3x4f _own_matrix = DiagonalMatrix<3, 4>(1.0f);
		Matrix3x4f* _matrix = nullptr;
		bool _read_only = false;

		bool _raw_view = true;

	public:

		ImGuiTransform3D() = default;

		ImGuiTransform3D(Matrix3x4f* ptr, bool read_only = false):
			_matrix(ptr),
			_read_only(read_only)
		{
			if (!_matrix)
			{
				_matrix = &_own_matrix;
			}
		}

		ImGuiTransform3D(const Matrix3x4f* ptr) :
			_matrix(const_cast<Matrix3x4f*>(ptr)),
			_read_only(true)
		{
			if (!_matrix)
			{
				_matrix = &_own_matrix;
			}
		}
		
		void setMatrixValue(Matrix3x4f const& m)
		{
			_own_matrix = m;
		}

		void bindMatrix(nullptr_t)
		{
			_matrix = &_own_matrix;
		}

		void bindMatrix(nullptr_t, bool read_only)
		{
			_matrix = &_own_matrix;
			_read_only = read_only;
		}

		void bindMatrix(Matrix3x4f* ptr, bool read_only = false)
		{
			if (ptr)
			{
				_matrix = ptr;
				_own_matrix = *_matrix;
			}
			else
			{
				_matrix = &_own_matrix;
			}
			_read_only = read_only;
		}

		void bindMatrix(const Matrix3x4f* ptr)
		{
			bindMatrix(const_cast<Matrix3x4f*>(ptr), true);
		}

		bool declare();

		Matrix3x4f getMatrix() const
		{
			return *_matrix;
		}

	};

	struct StringFilter
	{
		std::string filter = {};
		bool case_sensitive = false;

		static bool Accepts(std::string_view filter, std::string_view candidate, bool case_sensitive)
		{
			if (case_sensitive)
			{
				return std::contains(candidate, filter);
			}
			else
			{
				return std::containsCaseInsensitive(candidate, filter);
			}
		}

		bool accepts(std::string_view candidate) const
		{
			return Accepts(filter, candidate, case_sensitive);
		}

		bool rejects(std::string_view sv) const
		{
			return !accepts(sv);
		}

		constexpr bool empty() const
		{
			return filter.empty();
		}

		constexpr operator bool() const
		{
			return !empty();
		}

	};
}

struct ImRect;

namespace ImGui
{

	template <class Scalar>
	static constexpr ImGuiDataType GetDataType() noexcept
	{
		ImGuiDataType type = ImGuiDataType_COUNT;
		constexpr const size_t type_size_index = std::countr_zero(sizeof(Scalar));
		if constexpr (std::signed_integral<Scalar>)
		{
			type = ImGuiDataType_S8 + 2 * type_size_index;
		}
		else if constexpr (std::unsigned_integral<Scalar>)
		{
			type = ImGuiDataType_U8 + 2 * type_size_index;
		}
		else if constexpr (std::floating_point<Scalar>)
		{
			type = ImGuiDataType_Float + (type_size_index - 2);
		}
		else if constexpr (std::is_same<Scalar, bool>::value)
		{
			type = type = ImGuiDataType_Bool;
		}
		return type;
	}

	extern bool DragAngle(const char* label, float* v_rad, float v_speed = 1, float v_degrees_min = -180, float v_degrees_max = 180, const char* format = "%.1f", ImGuiSliderFlags flags = 0);

	extern bool SliderAngleN(const char* label, float* v_rad, uint N, float v_degrees_min = -180, float v_degrees_max = 180, const char* format = "%.1f", ImGuiSliderFlags flags = 0, uint8_t* changed_bit_field = nullptr);

	// Returns a bit field (bit N -> axis N changed)
	static inline uint SliderAngle3(const char* label, float* v_rad, float v_degrees_min = -180, float v_degrees_max = 180, const char* format = "%.1f", ImGuiSliderFlags flags = 0)
	{
		uint res = 0;
		bool b = SliderAngleN(label, v_rad, 3, v_degrees_min, v_degrees_max, format, flags, reinterpret_cast<uint8_t*>(&res));
		assert((res != 0) == b);
		return res;
	}

	template <class Scalar, uint R, uint C, int Options>
		requires (((Options& Eigen::RowMajor) != 0))
	static bool DragMatrix(const char* label, ::vkl::Matrix<Scalar, R, C, Options>& matrix, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		bool res = false;
		for (uint i = 0; i < matrix.rows(); ++i)
		{
			PushID(i);
			res |= DragScalarN(label, GetDataType<Scalar>(), matrix.row(i).data(), matrix.cols(), 1.0f, nullptr, nullptr, format, flags);
			PopID();
		}
		return res;
	}


	using ButtonIconDrawFunction = void(*)(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern bool IconButtonEx(const char* str_id, ImVec2 size, ImGuiButtonFlags flags, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data = nullptr);

	static inline bool IconButton(const char* str_id, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data = nullptr, bool small_button = false)
	{
		float sz = small_button ? ImGui::GetTextLineHeight() : ImGui::GetFrameHeight();
		return IconButtonEx(str_id, ImVec2(sz, sz), ImGuiButtonFlags_None, render_frame_fn, render_frame_data);
	}

	static inline bool SquareButton(const char* label, bool small_button=false)
	{
		float sz = small_button ? ImGui::GetTextLineHeight() : ImGui::GetFrameHeight();
		return Button(label, ImVec2(sz, sz));
	}

	extern void DrawRectNoCorners(ImDrawList* draw_list, ImRect rect, ImU32 color, float thickness = 1);

	extern void RenderDetachIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderXCrossIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderEyeIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderBarredIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern void RenderBarredEyeIcon(const void* p_data, ImDrawList* draw_list, ImRect const& rect, float font_size, ImU32 color);

	extern bool DetachButton();

	extern bool XCrossButton(const char* tooltip=nullptr, bool small_button = false);

	extern void LabelText2(const char* label, const char* text, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);

	static inline void TextBox(const char* label, const char* text, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
	{
		ImGui::LabelText2(label, text, flags);
	}

	extern void LabelValueEx(const char* label, ImGuiDataType type, const void* value, const char* fmt = nullptr);

	extern int PushReadOnlyDisabledStyleCol(ImGuiCol_ col = ImGuiCol_FrameBg);

	template <class Scalar>
	static void LabelValue(const char* label, Scalar const& value, const char* fmt = nullptr)
	{
		LabelValueEx(label, GetDataType<Scalar>(), &value, fmt);
	}

	// cursor must point to an array of at least 9 chars ("0x%016llx")
	static constexpr char* WriteFormatHex(char* cursor, char size_of_type, bool full_width = false, bool caps = false)
	{
		auto write = [&](const char* lit)
			{
				//cursor = strcpy(cursor, lit);
				while (*lit)
				{
					*(cursor++) = *(lit++);
				}
			};
		write("0x%");
		if (full_width)
		{
			*cursor++ = '0';
			cursor = std::format_to(cursor, "{}", 2 * size_of_type);
		}
		if (size_of_type == 2)
		{
			*cursor++ = 'h';
		}
		else if (size_of_type == 8)
		{
			write("ll");
		}
		*cursor++ = caps ? 'X' : 'x';
		return cursor;
	}

	template <std::integral UInt>
	static void LabelHexValue(const char* label, UInt value, bool full_width = false, bool caps = false)
	{
		std::array<char, 16> fmt;
		*WriteFormatHex(fmt.data(), sizeof(UInt), full_width, caps) = char(0);
		ImGui::LabelValue(label, value, fmt.data());
	}

	// const char* label
	// bool bit (bit field bool, cannot be passed as ref to a regular function)
	// returns true if changed, false otherwise
#define IMGUI_DECLARE_BIT_CHECKBOX(label, bit) \
[&]() { \
	bool _bit = bit; \
	bool res = ImGui::Checkbox(label, &_bit); \
	if(res) \
	{ \
		bit = _bit; \
	} \
	return res; \
}()

	template <std::integral Int>
	bool CheckboxBit(const char* label, Int& bitfield, unsigned int bit_index)
	{
		Int flag = Int(1) << Int(bit_index);
		bool bit = (bitfield & flag);
		bool res = ImGui::Checkbox(label, &bit);
		if (res)
		{
			if (bit)
			{
				bitfield |= flag;
			}
			else
			{
				bitfield &= ~flag;
			}
		}
		return res;
	}

	// Display a bool with a checkbox (read only)
	extern void LabelCheckbox(const char* label, bool b);

	// Same as InputTextWithHing, with an extra XCross button on the right side of the text field to clear the str
	extern bool TextFieldEdit(const char* label, std::string* str, const char* hint = nullptr, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);

	extern bool DeclareFilter(std::string* str, bool* p_case_sensitive, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);

	static inline bool DeclareFilter(vkl::StringFilter& filter, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
	{
		return DeclareFilter(&filter.filter, &filter.case_sensitive, flags);
	}

	// Warning: may not work for all scalar types yet! (relies on ImGui::SliderScalar to be implemented for the ImGuiDataType of the scalar)
	template <class Scalar>
	static bool SliderScalar(const char* label, Scalar& value, Scalar const& v_min, Scalar const& v_max, const char* fmt = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_None)
	{
		return ImGui::SliderScalar(label, GetDataType<Scalar>(), &value, &v_min, &v_max, fmt, flags);
	}
	
	extern bool InboxCheckboxEx(const char* label, bool* v, ImVec2 box_size, ButtonIconDrawFunction render_frame_fn = nullptr, const void* render_frame_data = nullptr);

	// Explicit sizing of the box
	extern bool InboxCheckbox(const char* label, bool* v, ImVec2 box_size);

	// Implicit sizing of the box
	extern bool InboxCheckbox(const char* label, bool* v, bool small_box=false);

	// Explicit sizing of the box
	extern bool IconCheckbox(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn, const void* render_frame_data, ImVec2 box_size);

	// Implicit sizing of the box
	extern bool IconCheckbox(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn = nullptr, const void* render_frame_data = nullptr, bool small_box = false);

	extern bool FlipIconButton(const char* label, bool* v, ImVec2 box_size,
		ButtonIconDrawFunction false_render_frame_fn = nullptr, const void* false_render_frame_data = nullptr,
		ButtonIconDrawFunction true_render_frame_fn = nullptr, const void* true_render_frame_data = nullptr
	);

	extern bool FlipIconButton(const char* label, bool* v,
		ButtonIconDrawFunction false_render_frame_fn = nullptr, const void* false_render_frame_data = nullptr,
		ButtonIconDrawFunction true_render_frame_fn = nullptr, const void* true_render_frame_data = nullptr,
		bool small_button = false
	);

	static inline bool FlipIconButton(const char* label, bool* v,
		ButtonIconDrawFunction false_render_frame_fn = nullptr,
		ButtonIconDrawFunction true_render_frame_fn = nullptr,
		bool small_button = false
	) {
		return FlipIconButton(label, v, false_render_frame_fn, nullptr, true_render_frame_fn, nullptr, small_button);
	}

	extern bool BarredIconButton(const char* label, bool* v, ButtonIconDrawFunction render_frame_fn = nullptr, const void* render_frame_data = nullptr, bool small_box = false, bool x_cross = false);


	extern bool ArrowFlipButton(const char* label_id);
}