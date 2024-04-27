#include "GuiContext.hpp"

namespace vkl
{
	std::shared_ptr<GUIStyle> g_default_style = [](){
		using Color = GUIStyle::Color;
		std::shared_ptr<GUIStyle> res = std::make_shared<GUIStyle>(GUIStyle{
			.valid_green = Color(0, 0.8, 0, 1),
			.invalid_red = Color(0.8, 0, 0, 1),
			.warning_yellow = Color(0.8, 0.8, 0, 1),
		});
		return res;
	}();



	GuiContext::GuiContext(CreateInfo const& ci) :
		_imgui_context(ci.imgui_context),
		_style(ci.style ? ci.style : g_default_style)
	{

	}
}