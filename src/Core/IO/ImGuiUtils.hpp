#pragma once

#include <vector>
#include <string>
#include "imgui.h"
#include <Core/Rendering/Math.hpp>

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
			std::string name;
			std::string desc;
		};

	protected:

		std::string _name = {};
		
		Mode _mode = Mode::Combo;

		size_t _index = 0;
		std::vector<Option> _options = {};
		
		bool _same_line;

	public:

		struct CreateInfo
		{
			std::string name = {};
			Mode mode = Mode::Combo;
			// labels xor options
			std::vector<std::string> labels;
			// labels xor options
			std::vector<Option> options;
			size_t default_index = 0;
			bool same_line = false;
		};
		using CI = CreateInfo;

		ImGuiListSelection() = default;

		ImGuiListSelection(CreateInfo const& ci);

		ImGuiListSelection& operator=(ImGuiListSelection const&) = default;
		ImGuiListSelection& operator=(ImGuiListSelection&&) = default;
		

		bool declareRadioButtons(bool same_line);
		
		bool declareRadioButtons()
		{
			return declareRadioButtons(_same_line);
		}

		bool declareCombo();

		bool declareDropdown()
		{
			return declareCombo();
		}

		bool declare()
		{
			if (_mode == Mode::RadioButtons)
			{
				return declareRadioButtons(_same_line);
			}
			else if (_mode == Mode::Combo)
			{
				return declareCombo();
			}
			return false;
		}

		constexpr size_t index()const
		{
			return _index;
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