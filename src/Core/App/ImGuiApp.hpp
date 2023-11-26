#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/VkObjects/VkWindow.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <Core/IO/GuiContext.hpp>
#include <cassert>

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

		GuiContext _gui_context;

		GuiContext * beginImGuiFrame()
		{			
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			return &_gui_context;
		}

		void endImGuiFrame(GuiContext * ctx)
		{
			assert(ctx == &_gui_context);
			ImGui::EndFrame();
			ImGui::UpdatePlatformWindows();
		}

	public:

		void initImGui(std::shared_ptr<VkWindow> const& main_window);

		struct CreateInfo
		{
			std::string name = {};
			bool enable_validation = false;
		};
		using CI = CreateInfo;

		AppWithWithImGui(CreateInfo const& ci);

		virtual ~AppWithWithImGui() override;

	};
}