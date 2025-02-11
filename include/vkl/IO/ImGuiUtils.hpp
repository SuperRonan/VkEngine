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

}
	
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

	bool SliderAngleN(const char* label, float * v_rad, uint N, float v_degrees_min=-180, float v_degrees_max=180, const char * format = "%.1f", ImGuiSliderFlags flags = 0, uint8_t* changed_bit_field = nullptr);

	// Returns a bit field (bit N -> axis N changed)
	static inline uint SliderAngle3(const char* label, float* v_rad, float v_degrees_min = -180, float v_degrees_max = 180, const char* format = "%.1f", ImGuiSliderFlags flags = 0)
	{
		uint res = 0;
		bool b = SliderAngleN(label, v_rad, 3, v_degrees_min, v_degrees_max, format, flags, reinterpret_cast<uint8_t*>(&res));
		assert((res != 0) == b);
		return res;
	}

	template <class Scalar, uint R, uint C, int Options>
		requires (((Options & Eigen::RowMajor) != 0))
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
}