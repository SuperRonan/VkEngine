#pragma once

#include <Core/App/MainWindowApp.hpp>
#include <Core/VkObjects/VkWindow.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl2.h>
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

		ImGuiConfigFlags _imgui_init_flags = 0;

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
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			return &_gui_context;
		}

		void endImGuiFrame(GuiContext * ctx)
		{
			assert(ctx == &_gui_context);
			ImGui::EndFrame();
			if (_imgui_init_flags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
			}
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

		ImGuiConfigFlags imguiConfigFlags() const
		{
			return _imgui_init_flags;
		}
	};
}