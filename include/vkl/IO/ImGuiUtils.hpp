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
		
		using Mat4x3 = Matrix4x3f;
		using Mat3x4 = Matrix3x4f;

		Mat4x3 _own_matrix = Mat4x3(1);
		Mat4x3 * _matrix = nullptr;
		bool _read_only = false;

		bool _raw_view = true;

	public:

		ImGuiTransform3D() = default;

		ImGuiTransform3D(Mat4x3 * ptr, bool read_only = false):
			_matrix(ptr),
			_read_only(read_only)
		{
			if (!_matrix)
			{
				_matrix = &_own_matrix;
			}
		}

		ImGuiTransform3D(const Mat4x3* ptr) :
			_matrix(const_cast<Mat4x3*>(ptr)),
			_read_only(true)
		{
			if (!_matrix)
			{
				_matrix = &_own_matrix;
			}
		}
		
		void setMatrixValue(Mat4x3 const& m)
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

		void bindMatrix(Mat4x3* ptr, bool read_only = false)
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

		void bindMatrix(const Mat4x3* ptr)
		{
			bindMatrix(const_cast<Mat4x3*>(ptr), true);
		}

		bool declare();

		Mat4x3 getMatrix() const
		{
			return *_matrix;
		}

	};

}
	
namespace ImGui
{
	
	bool SliderAngleN(const char* label, float * v_rad, uint N, float v_degrees_min=-180, float v_degrees_max=180, const char * format = "%.1f", ImGuiSliderFlags flags = 0, uint8_t* changed_bit_field = nullptr);

	// Returns a bit field (bit N -> axis N changed)
	static inline uint SliderAngle3(const char* label, float* v_rad, float v_degrees_min = -180, float v_degrees_max = 180, const char* format = "%.1f", ImGuiSliderFlags flags = 0)
	{
		uint res = 0;
		bool b = SliderAngleN(label, v_rad, 3, v_degrees_min, v_degrees_max, format, flags, reinterpret_cast<uint8_t*>(&res));
		assert((res != 0) == b);
		return res;
	}

	static inline bool DragMatrix4x3(const char* label, ::vkl::Matrix4x3f& matrix, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		::vkl::Matrix3x4f t = glm::transpose(matrix);
		bool res = false;
		for (uint i = 0; i < 3; ++i)
		{
			ImGui::PushID(i);
			res |= DragFloat4("", &t[i].x, 1, 0, 0, format, flags);
			ImGui::PopID();
		}
		if (res)
		{
			matrix = glm::transpose(t);
		}
		return res;
	}

	static inline void DragMatrix4x3(const char* label, const ::vkl::Matrix4x3f& matrix, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		ImGui::BeginDisabled();
		DragMatrix4x3(label, const_cast<::vkl::Matrix4x3f&>(matrix), format, flags);
		ImGui::EndDisabled();
	}
}