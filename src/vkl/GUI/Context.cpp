#define IMGUI_DEFINE_MATH_OPERATORS 1
#include <vkl/GUI/Context.hpp>

namespace vkl::GUI
{
	std::shared_ptr<Style> g_default_style = [](){
		using Color = Style::Color;
		Color default_border_color = Color(0.50f, 0.50f, 0.50f, 0.50f);
		std::shared_ptr<Style> res = std::make_shared<Style>(Style{
			.valid_green = Color(0, 0.8, 0, 1),
			.invalid_red = Color(0.8, 0, 0, 1),
			.warning_yellow = Color(0.8, 0.8, 0, 1),
			.stack_colors = {
				default_border_color,
				default_border_color * ImVec4(1.5, 0.5, 0.5, 1),
				default_border_color * ImVec4(0.5, 1.5, 0.5, 1),
				default_border_color * ImVec4(0.5, 0.5, 1.5, 1),
			},
		});
		return res;
	}();



	Context::Context(CreateInfo const& ci) :
		_imgui_context(ci.imgui_context),
		_style(ci.style ? ci.style : g_default_style),
		_common_file_dialog(ci.common_file_dialog)
	{
		if (!_common_file_dialog)
		{
			_common_file_dialog = std::make_shared<FileDialog>(FileDialog::CI{

			});
		}
	}

	void Context::begin()
	{
		if (!_keep_drag_drop_payload)
		{
			_drag_drop_payload.object.reset();
			_keep_drag_drop_payload = false;
		}
	}

	void Context::end()
	{

	}

	void Context::pushPanelHolder(PanelHolder* panel)
	{
		_panel_holder_stack.push_back(panel);
	}

	void Context::popPanelHolder()
	{
		_panel_holder_stack.pop_back();
	}

	PanelHolder* Context::getTopPanelHolder(uint index) const
	{
		return _panel_holder_stack.at(_panel_holder_stack.size32() - index - 1);
	}

	PanelHolder* Context::getBottomPanelHolder(uint index) const
	{
		return _panel_holder_stack.at(index);
	}

	void Context::pushPanel(Panel* panel)
	{
		_panel_stack.push_back(panel);
	}

	void Context::popPanel()
	{
		_panel_stack.pop_back();
	}

	Panel* Context::getTopPanel(uint index) const
	{
		return _panel_stack.at(_panel_stack.size32() - index - 1);
	}

	Panel* Context::getBottomPanel(uint index) const
	{
		return _panel_stack.at(index);
	}

	std::span<PanelHolder* const> Context::getPanelHolderStack() const
	{
		return std::span<PanelHolder* const>(_panel_holder_stack.data(), _panel_holder_stack.size());
	}

	std::span<Panel* const> Context::getPanelStack() const
	{
		return std::span<Panel* const>(_panel_stack.data(), _panel_stack.size());
	}

	Style::Color Context::pushStack()
	{
		const auto& colors = style().stack_colors;
		if (colors.empty())
		{
			++_stack_counter;
			return ImGui::GetStyleColorVec4(ImGuiCol_Border);
		}
		Style::Color res = colors[_stack_counter % static_cast<uint32_t>(colors.size())];
		++_stack_counter;
		return res;
	}

	Style::Color Context::popStack()
	{
		--_stack_counter;
		const auto& colors = style().stack_colors;
		if (colors.empty())
		{
			return ImGui::GetStyleColorVec4(ImGuiCol_Border);
		}
		Style::Color res = colors[_stack_counter % static_cast<uint32_t>(colors.size())];
		return res;
	}

	void Context::clearTemporaryData()
	{
		_clipboard_payload.object.reset();
	}
}