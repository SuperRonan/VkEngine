#pragma once

#include <vkl/App/MainWindowApp.hpp>
#include <vkl/VkObjects/VkWindow.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <vkl/GUI/Context.hpp>
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

		GUI::Context _gui_context;

		bool _enable_imgui = false;

		GUI::Context * beginImGuiFrame()
		{			
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL3_NewFrame();
			ImGui::NewFrame();
			return &_gui_context;
		}

		void endImGuiFrame(GUI::Context * ctx)
		{
			assert(ctx == &_gui_context);
			ImGui::EndFrame();
#ifdef IMGUI_HAS_VIEWPORT
			if (_imgui_init_flags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
			}
#endif
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

		bool ImGuiIsEnabled()const
		{
			return _enable_imgui;
		}

		bool hasImGuiViewports() const
		{
			bool res = false;
#ifdef IMGUI_HAS_VIEWPORT
			res = (_imgui_init_flags & ImGuiConfigFlags_ViewportsEnable);
#endif
			return res;
		}

		bool hasImGuiDocking() const
		{
			bool res = false;
#ifdef IMGUI_HAS_DOCK
			res = (_imgui_init_flags & ImGuiConfigFlags_DockingEnable);
#endif
			return res;
		}

		bool ImGuiIsInit() const
		{
			return !!_imgui_ctx;
		}
	};
}