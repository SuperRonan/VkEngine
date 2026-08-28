#pragma once

#include <vkl/App/MainWindowApp.hpp>
#include <vkl/VkObjects/VkWindow.hpp>

#include <imgui/imgui.h>

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

		//struct ImGuiWindow
		//{
		//	std::shared_ptr<VkWindow> _window = nullptr;
		//	ImGuiViewport * _viewport = nullptr;
		//};

		ImGuiContext* _imgui_ctx = nullptr;
		//std::vector<ImGuiWindow *> _imgui_windows = {};

		GUI::Context _gui_context;

		bool _enable_imgui = false;
		bool _enable_main_window_docking = false;
		bool _main_viewport_is_reduced = false;

		ImGuiDockNodeFlags _main_dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingOverCentralNode;
		ImGuiID _main_dockspace_id = 0;
		Vector2u _main_offset = Vector2u::Zero();
		Vector2u _main_resolution; // May be reduced by the docking

		GUI::Context* beginImGuiFrame();

		void endImGuiFrame(GUI::Context * ctx);

		GUI::Context* getLatestGUIContext()
		{
			return _enable_imgui ? &_gui_context : nullptr;
		}

	public:

		void initImGui();

		void setEnableMainWindowDocking(bool enable = true);

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

		Vector2u mainViewportResolution() const
		{
			if (_main_viewport_is_reduced)
			{
				return _main_resolution;
			}
			else
			{
				VkExtent2D extent = _main_window->extent2D().value();
				return Vector2u(extent.width, extent.height);
			}
		}

		Vector2u mainViewportOffset() const
		{
			return _main_offset;
		}

		bool isMainViewportReduced() const
		{
			return _main_viewport_is_reduced;
		}
	};
}