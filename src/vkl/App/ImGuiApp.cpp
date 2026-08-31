#define IMGUI_DEFINE_MATH_OPERATORS 1
#include <vkl/App/ImGuiApp.hpp>
#include <cassert>

#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>

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
		ImGui::GetIO().ConfigFlags |= (_imgui_init_flags | ImGuiConfigFlags_IsSRGB);
		ImGui::GetIO().ConfigViewportsNoDefaultParent = true;
		
		_gui_context = GUI::Context::CI{
			.imgui_context = _imgui_ctx,
		};

		_enable_imgui = true;

		ImGuiPlatformIO& pio = ImGui::GetPlatformIO();

		// The default color palette is in sRGB
		// Convert it to linear (the shader will do the proper color correction)
		auto& style = ImGui::GetStyle();
		for (uint i = 0; i < ImGuiCol_COUNT; ++i)
		{
			for (uint c = 0; c < 3; ++c)
			{
				float& x = *(&(style.Colors[i].x) + c);
				// TODO properly
				x = std::pow(x, 2.4f);
				//x = ColorCorrectionCommon::sRGB_OETF(x);
			}
		}

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
		ImGui_ImplSDL3_InitForVulkan(_main_window->handle());
		//ImGui_ImplWin32_Init(glfwGetWin32Window(main_window->handle()));
	}

	AppWithImGui::AppWithImGui(CreateInfo const& ci) :
		MainWindowApp(MainWindowApp::CI{
			.name = ci.name,
			.args = ci.args,
		}),
		_gui_context(GUI::Context::CI{
			.imgui_context = nullptr,
		})
	{
		assert(!g_app);
		g_app = this;

		_imgui_init_flags |= ImGuiConfigFlags_NavEnableKeyboard;
		
#ifdef IMGUI_HAS_DOCK
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
#endif

#ifdef IMGUI_HAS_VIEWPORT
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
#endif
	}

	void AppWithImGui::setEnableMainWindowDocking(bool enable)
	{
		bool can_dock = hasImGuiDocking();
		if (can_dock)
		{
			_enable_main_window_docking = enable;
		}
	}

	constexpr const uint DockNodeChildCount = IM_COUNTOF(ImGuiDockNode::ChildNodes);

	ImGuiDockNode* FindMainViewportDockNode(ImGuiDockNode* node)
	{
		if (node->IsSplitNode())
		{
			ImGuiDockNode* res = nullptr;
			
			for (uint i = 0; i < DockNodeChildCount; ++i)
			{
				res = FindMainViewportDockNode(node->ChildNodes[i]);
				if (res)
				{
					break;
				}
			}
			return res;
		}
		else if (node->VisibleWindow == nullptr && node->Size.x > 0 && node->Size.y > 0) // Best criteria so far
		{
			return node;
		}
		return nullptr;
	}

	bool DockNodeTreeHasAnyWindow(ImGuiDockNode* node)
	{
		if (node->IsLeafNode())
		{
			return !node->Windows.empty();
		}
		for (uint i = 0; i < DockNodeChildCount; ++i)
		{
			if (node->ChildNodes[i] && DockNodeTreeHasAnyWindow(node->ChildNodes[i]))
			{
				return true;
			}
		}
		return false;
	}

	bool DockNodeTreeIsSane(ImGuiDockNode* node)
	{
		return node->IsLeafNode() || DockNodeTreeHasAnyWindow(node);
	}

	bool DockNodeTreeIsDegenerate(ImGuiDockNode* node)
	{
		return !DockNodeTreeIsSane(node);
	}

	ImGuiDockNode* FindTopMost(ImGuiDockNode* node)
	{
		while(!node->IsRootNode())
		{
			node = node->ParentNode;
		}
		return node;
	}

	GUI::Context* AppWithImGui::beginImGuiFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		_gui_context.begin();
		{
			VkExtent2D extent = _main_window->extent2D().value();
			_main_resolution = Vector2u(extent.width, extent.height);
			_main_offset = Vector2u::Zero();
			_main_viewport_is_reduced = false;
		}
#ifdef IMGUI_HAS_DOCK
		if (_enable_main_window_docking)
		{
			ImGuiViewport* main_viewport = ImGui::GetMainViewport();
			_main_dockspace_id = ImGui::DockSpaceOverViewport(_main_dockspace_id, main_viewport, _main_dockspace_flags);
			ImGuiDockNode* const top_node = ImGui::DockContextFindNodeByID(_imgui_ctx, _main_dockspace_id);
			if (DockNodeTreeIsDegenerate(top_node))
			{
				const bool do_clear = true;
				if (do_clear)
				{
					ImGui::DockContextClearNodes(_imgui_ctx, _main_dockspace_id, true);
					_main_dockspace_id = 0;
				}
			}
			else
			{
				ImVec2 window_size = ImGui::GetWindowSize();
				ImGuiDockNode* const node = FindMainViewportDockNode(top_node);
				if (node && node != top_node)
				{
					ImVec2 resolution = node->Size;
					_main_resolution = Vector2u(resolution.x, resolution.y);
					// node->Pos is absolute, not relative to the viewport
					ImVec2 offset = node->Pos - main_viewport->Pos;
					_main_offset = Vector2u(offset.x, offset.y);
					_main_viewport_is_reduced = true;
				}
			}

			//if (ImGui::Begin("Debug Main Viewport Dock"))
			//{
			//	ImGui::InputInt2("Offset", reinterpret_cast<int*>(_main_offset.data()));
			//	ImGui::InputInt2("Resolution", reinterpret_cast<int*>(_main_resolution.data()));
			//	ImGui::Checkbox("Reduced", &_main_viewport_is_reduced);
			//}
			//ImGui::End();
		}
#endif
		return &_gui_context;
	}

	void AppWithImGui::endImGuiFrame(GUI::Context* ctx)
	{
		assert(ctx == &_gui_context);
		ctx->end();
		ImGui::EndFrame();
#ifdef IMGUI_HAS_VIEWPORT
		if (_imgui_init_flags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
		}
#endif
	}

	AppWithImGui::~AppWithImGui()
	{
		if (ImGuiIsEnabled())
		{
			ImGui_ImplSDL3_Shutdown();

			if (_imgui_ctx)
			{
				ImGui::DestroyContext(_imgui_ctx);
				_imgui_ctx = nullptr;
			}

			g_app = nullptr;
		}
	}
}