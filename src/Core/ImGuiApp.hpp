#pragma once

#include "VkApplication.hpp"
#include "VkWindow.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

namespace vkl
{
	class AppWithWithImGui : public VkApplication
	{
	protected:

		static AppWithWithImGui* g_app;

		struct ImGuiWindow
		{
			std::shared_ptr<VkWindow> _window = nullptr;
			ImGuiViewport * _viewport = nullptr;
		};

		ImGuiContext* _imgui_ctx = nullptr;
		std::vector<ImGuiWindow *> _imgui_windows = {};

		VkPresentModeKHR _present_mode = VK_PRESENT_MODE_FIFO_KHR;

	public:

		void initImGui(std::shared_ptr<VkWindow> const& main_windo);

		struct CreateInfo
		{
			std::string name = {};
			bool enable_validation = false;
			VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
		};
		using CI = CreateInfo;

		AppWithWithImGui(CreateInfo const& ci);

		virtual ~AppWithWithImGui() override;

	};
}