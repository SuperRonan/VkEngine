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

		bool declareRadioButtons(size_t & active_index, bool same_line) const;
		
		bool declareRadioButtons(bool same_line)
		{
			return declareRadioButtons(_index, same_line);
		}
		
		bool declareRadioButtons()
		{
			return declareRadioButtons(_same_line);
		}

		bool declareCombo(size_t & acive_index) const;
		
		bool declareCombo()
		{
			return declareCombo(_index);
		}

		bool declareDropdown(size_t & active_index)
		{
			return declareCombo(_index);
		}

		bool declareDropdown()
		{
			return declareDropdown();
		}

		bool declare(size_t & active_index)
		{
			if (_mode == Mode::RadioButtons)
			{
				return declareRadioButtons(active_index, _same_line);
			}
			else if (_mode == Mode::Combo)
			{
				return declareCombo(active_index);
			}
			return false;
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