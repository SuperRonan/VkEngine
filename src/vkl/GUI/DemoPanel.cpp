#include <vkl/GUI/DemoPanel.hpp>

#include <imgui/imgui.h>

namespace vkl::GUI
{
	std::shared_ptr<DemoPanel> DemoPanel::_singleton = {};

	std::shared_ptr<DemoPanel> DemoPanel::GetSingleton()
	{
		if (!_singleton)
		{
			_singleton = std::shared_ptr<DemoPanel>(new DemoPanel());
		}
		return _singleton;
	}

	DemoPanel::DemoPanel(VkApplication * app):
		Panel(app, "Demo Panel")
	{ }

	DemoPanel::~DemoPanel()
	{

	}

	void DemoPanel::declare(Context& ctx)
	{
		ImGui::ShowDemoWindow(&_open);
	}
}