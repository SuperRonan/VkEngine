#include <vkl/GUI/PathWidget.hpp>

#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/PanelHolder.hpp>

#include <vkl/App/VkApplication.hpp>

#include <imgui/imgui_internal.h>

namespace vkl::GUI
{
	bool PathWidget::declareInline(Context& ctx)
	{
		bool res = false;
		const char* plabel = label.empty() ? "Path" : label.c_str();
		res |= ImGui::TextFieldEdit(plabel, &path_string, "...", text_edit_flags);
		if (res)
		{
			path = path_string;
		}
		auto& file_dialog = ctx.getCommonFileDialog();
		bool can_open = file_dialog->canOpen() && ((text_edit_flags & ImGuiInputTextFlags_ReadOnly) == 0);
		{
			ImGuiContext& g = *GImGui;
			if (g.CurrentItemFlags & (ImGuiItemFlags_Disabled | ImGuiItemFlags_ReadOnly))
			{
				can_open &= false;
			}
		}
		const void* owner = this->owner ? this->owner : this;
		ImGui::SameLine();
		ImGui::BeginDisabled(!can_open);
		if (ImGui::Button("..."))
		{
			FileDialog::OpenInfo open_info{
				.filters = filters,
				.parent_window = ctx.getCurrentWindow(),
				.allow_multiple = false,
				.mode = mode,
			};
			VkApplication* app = ctx.getTopPanelHolder()->application();
			FileSystem& fs = *app->fileSystem();
			auto resolved = fs.resolve(path);
			if (resolved.result == that::Result::Success)
			{
				resolved = fs.cannonize(resolved.value);
				if (resolved.result == that::Result::Success)
				{
					open_info.default_location = resolved.value;
				}
			}
			file_dialog->open(owner, open_info);
		}
		ImGui::EndDisabled();
		if (file_dialog->owner() == owner)
		{
			if (file_dialog->completed())
			{
				auto results = file_dialog->getResults();
				if (!results.empty())
				{
					auto file_dialog_path = file_dialog->getResults().front();
					setPath(file_dialog_path);
					res |= true;
				}
				file_dialog->close();
			}
		}
		return res;
	}
}