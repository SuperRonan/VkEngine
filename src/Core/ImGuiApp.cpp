#include "ImGuiApp.hpp"
#include <cassert>

namespace vkl
{
	AppWithWithImGui * AppWithWithImGui::g_app;

	void AppWithWithImGui::initImGui(std::shared_ptr<VkWindow> const& main_window)
	{
		_imgui_ctx = ImGui::CreateContext();
		ImGui::SetCurrentContext(_imgui_ctx);
		
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

		ImGuiPlatformIO& pio = ImGui::GetPlatformIO();

		

		pio.Renderer_CreateWindow = [](ImGuiViewport* _vp)
		{
			ImGuiViewport& vp = *_vp;
			ImGuiWindow* w = new ImGuiWindow;
			vp.RendererUserData = w;
			w->_viewport = _vp;


			VkWindow::CreateInfo window_ci{
				.app = g_app,
				.queue_families_indices = std::set({g_app->_queue_family_indices.graphics_family.value(), g_app->_queue_family_indices.present_family.value()}),
				.target_present_mode = g_app->_present_mode,
				.name = "",
				.w = static_cast<uint32_t>(vp.Size.x),
				.h = static_cast<uint32_t>(vp.Size.y),
				.resizeable = GLFW_TRUE,
			};
			w->_window = std::make_shared<VkWindow>(window_ci);
			
			g_app->_imgui_windows.push_back(w);
		};

		pio.Renderer_DestroyWindow = [](ImGuiViewport* _vp)
		{
			ImGuiViewport& vp = *_vp;
			ImGuiWindow* w = reinterpret_cast<ImGuiWindow*>(vp.RendererUserData);

			auto it = g_app->_imgui_windows.begin();
			const auto end = g_app->_imgui_windows.end();

			while (it != end)
			{
				if ((*it)->_viewport == _vp)
				{
					g_app->_imgui_windows.erase(it);
					break;
				}
				++it;
			}
		};

		pio.Renderer_SetWindowSize = [](ImGuiViewport* _vp, ImVec2 size)
		{
			ImGuiViewport& vp = *_vp;
			ImGuiWindow* w = reinterpret_cast<ImGuiWindow*>(vp.RendererUserData);
			
			w->_window->setSize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
		};

		ImGui_ImplGlfw_InitForVulkan(*main_window, true);
	}

	AppWithWithImGui::AppWithWithImGui(CreateInfo const& ci) :
		VkApplication(ci.name, ci.enable_validation),
		_present_mode(ci.present_mode)
	{
		assert(!g_app);
		g_app = this;
	}

	AppWithWithImGui::~AppWithWithImGui()
	{
		if (_imgui_ctx)
		{
			ImGui::DestroyContext(_imgui_ctx);
			_imgui_ctx = nullptr;
		}

		g_app = nullptr;
	}
}