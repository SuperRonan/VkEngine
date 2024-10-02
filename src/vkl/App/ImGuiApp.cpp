#include <vkl/App/ImGuiApp.hpp>
#include <cassert>

#include <argparse/argparse.hpp>

namespace vkl
{
	
	void AppWithImGui::FillArgs(argparse::ArgumentParser& args)
	{
		MainWindowApp::FillArgs(args);
		
		args.add_argument("--imgui_docking")
			.help("Force the ImGui Docking feature (0 or 1)")
			.scan<'d', unsigned int>()
		;

		args.add_argument("--imgui_multi_viewport")
			.help("Force the ImGui Multi Viewport feature (0 or 1)")
			.scan<'d', unsigned int>();
	}


	
	AppWithImGui* AppWithImGui::g_app;

	void AppWithImGui::initImGui()
	{
		_imgui_ctx = ImGui::CreateContext();
		ImGui::SetCurrentContext(_imgui_ctx);
		ImGui::GetIO().ConfigFlags |= _imgui_init_flags;
		
		_gui_context = GuiContext::CI{
			.imgui_context = _imgui_ctx,
		};

		_enable_imgui = true;

		ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
		

		//pio.Renderer_CreateWindow = [](ImGuiViewport* _vp)
		//{
		//	ImGuiViewport& vp = *_vp;
		//	ImGuiWindow* w = new ImGuiWindow;
		//	vp.RendererUserData = w;
		//	w->_viewport = _vp;


		//	VkWindow::CreateInfo window_ci{
		//		.app = g_app,
		//		.queue_families_indices = std::set({g_app->_queue_family_indices.graphics_family.value(), g_app->_queue_family_indices.present_family.value()}),
		//		.name = "",
		//		.w = static_cast<uint32_t>(vp.Size.x),
		//		.h = static_cast<uint32_t>(vp.Size.y),
		//		.resizeable = GLFW_TRUE,
		//	};
		//	w->_window = std::make_shared<VkWindow>(window_ci);
		//	
		//	g_app->_imgui_windows.push_back(w);
		//};

		//pio.Renderer_DestroyWindow = [](ImGuiViewport* _vp)
		//{
		//	ImGuiViewport& vp = *_vp;
		//	ImGuiWindow* w = reinterpret_cast<ImGuiWindow*>(vp.RendererUserData);

		//	auto it = g_app->_imgui_windows.begin();
		//	const auto end = g_app->_imgui_windows.end();

		//	while (it != end)
		//	{
		//		if ((*it)->_viewport == _vp)
		//		{
		//			g_app->_imgui_windows.erase(it);
		//			break;
		//		}
		//		++it;
		//	}
		//};

		//pio.Renderer_SetWindowSize = [](ImGuiViewport* _vp, ImVec2 size)
		//{
		//	ImGuiViewport& vp = *_vp;
		//	ImGuiWindow* w = reinterpret_cast<ImGuiWindow*>(vp.RendererUserData);
		//	
		//	w->_window->setSize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
		//};
		assert(!!_main_window);
		ImGui_ImplSDL2_InitForVulkan(_main_window->handle());
		//ImGui_ImplWin32_Init(glfwGetWin32Window(main_window->handle()));
	}

	AppWithImGui::AppWithImGui(CreateInfo const& ci) :
		MainWindowApp(MainWindowApp::CI{
			.name = ci.name,
			.args = ci.args,
		}),
		_gui_context(GuiContext::CI{
			.imgui_context = nullptr,
		})
	{
		assert(!g_app);
		g_app = this;

		_imgui_init_flags |= ImGuiConfigFlags_NavEnableKeyboard;
		
		if (ci.args.is_used("--imgui_docking"))
		{
			if (ci.args.get<unsigned int>("--imgui_docking") == 1)
			{
				_imgui_init_flags |= ImGuiConfigFlags_DockingEnable;
			}
		}
		else
		{
			// Might be platform dependant
			_imgui_init_flags |= ImGuiConfigFlags_DockingEnable;
		}

		if (ci.args.is_used("--imgui_multi_viewport"))
		{
			if (ci.args.get<unsigned int>("--imgui_multi_viewport") == 1)
			{
				_imgui_init_flags |= ImGuiConfigFlags_ViewportsEnable;
			}
		}
		else
		{
			// Might be platform dependant
			_imgui_init_flags |= ImGuiConfigFlags_ViewportsEnable;
		}
	}

	AppWithImGui::~AppWithImGui()
	{
		if (ImGuiIsEnabled())
		{
			ImGui_ImplSDL2_Shutdown();

			if (_imgui_ctx)
			{
				ImGui::DestroyContext(_imgui_ctx);
				_imgui_ctx = nullptr;
			}

			g_app = nullptr;
		}
	}
}