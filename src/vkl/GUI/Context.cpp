#include <vkl/GUI/Context.hpp>

namespace vkl::GUI
{
	std::shared_ptr<Style> g_default_style = [](){
		using Color = Style::Color;
		std::shared_ptr<Style> res = std::make_shared<Style>(Style{
			.valid_green = Color(0, 0.8, 0, 1),
			.invalid_red = Color(0.8, 0, 0, 1),
			.warning_yellow = Color(0.8, 0.8, 0, 1),
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

	void Context::pushPanelHolder(PanelHolder* panel)
	{
		_panel_holder_stack.push_back(panel);
	}

	void Context::popPanelHolder()
	{
		_panel_holder_stack.pop_back();
	}

	PanelHolder* Context::getTopPanelHolder(uint index)
	{
		return _panel_holder_stack.at(_panel_holder_stack.size32() - index - 1);
	}

	PanelHolder* Context::getBottomPanelHolder(uint index)
	{
		return _panel_holder_stack.at(index);
	}
}