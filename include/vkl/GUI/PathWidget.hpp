#pragma once

#include <vkl/GUI/FileDialog.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/IO/FileSystem.hpp>

namespace vkl::GUI
{
	struct PathWidget
	{
		using Filter = FileDialog::Filter;
		std::string label = {};
		ImGuiInputTextFlags text_edit_flags = ImGuiInputTextFlags_None;
		FileDialog::Mode mode = {};
		FileSystem::Path path = {};
		std::string path_string = {};
		MyVector<Filter> filters = {};
		const void* owner = nullptr;

		void setPath(FileSystem::Path const& new_path)
		{
			path = new_path;
			path_string = path.string();
		}

		bool declareInline(Context& ctx);
	};
}