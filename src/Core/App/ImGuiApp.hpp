#pragma once

#include <Core/App/MainWindowApp.hpp>
#include <Core/VkObjects/VkWindow.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <Core/IO/GuiContext.hpp>
#include <cassert>

namespace vkl
{
	class AppWithImGui : public MainWindowApp
	{
	public:
		
		static void FillArgs(argparse::ArgumentParser & args_parser);

	protected:

		Options _options = {};

		static AppWithImGui* g_app;

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

		void initImGui();

		struct CreateInfo
		{
			std::string name = {};
			argparse::ArgumentParser & args;
		};
		using CI = CreateInfo;

		AppWithImGui(CreateInfo const& ci);

		virtual ~AppWithImGui() override;
	};
}