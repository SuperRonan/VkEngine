#include <vkl/GUI/DemoPanel.hpp>

#include <imgui/imgui.h>

namespace vkl::GUI
{
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