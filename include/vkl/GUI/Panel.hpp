#pragma once

#include <vkl/App/VkApplication.hpp>

#include <imgui/imgui.h>

namespace vkl::GUI
{
	class Panel : public VkObject
	{
	public:
		using Id = uintptr_t;
		using DeclareFunction = std::function<void(Context&, Panel*)>;

		static constexpr ImGuiID InvalidDockID = 0;

		enum class FocusAction : u8
		{
			None = 0,
			TakeFocus = 1,
			NoFocus = 2,
		};

	protected:

		bool _open : 1 = true;
		bool _can_close : 1 = true;
		bool _used : 1 = false;
		bool _is_visible : 1 = false;
		bool _is_hovered : 1 = false;
		bool _next_take_focus : 1 = false; // Consumed by next declare()
		bool _next_no_focus_on_appearing : 1 = false; // Consumed by next declare()
		bool _next_no_bring_to_front_on_focus : 1 = false; // Consumed by next declare()
		bool _has_focus : 1 = false;
		bool _disable_from_ctx_stack : 1 = false;
		bool _has_created_dock_node : 1 = false;
		bool _set_dock_id : 1 = false; // Consumed by next declare()
		bool _set_dock_id_always : 1 = false;
		bool _should_auto_resize_x : 1 = false; // Consumed by next declare()
		bool _should_auto_resize_y : 1 = false; // Consumed by next declare()
		bool _should_toggle_collapse : 1 = false; // Consumed by next declare()
		bool _should_collapse : 1 = false;

		ImVec2 _window_size = ImVec2(0, 0);
		ImGuiWindowFlags _window_flags = ImGuiWindowFlags_None;
		ImGuiID _dock_id = {};
		//ImGuiDockNode* _dock_node = {};

		Panel(VkApplication * app, std::string_view name);

	public:

		Panel(Panel&&) noexcept = default; 

		virtual ~Panel() override;

		void declarePanelControlMenu(Context& ctx);

		virtual void declareMenu(Context& ctx);

		virtual void declareInline(Context& ctx);

		virtual void declare(Context& ctx, bool keep_open=false);

		bool isOpen() const
		{
			return _open;
		}

		bool isUsed() const
		{
			return _used;
		}

		void setUsed(bool value = true)
		{
			_used = value;
		}

		void setOpen(bool value = true)
		{
			_open = value;
		}

		bool isVisible() const
		{
			return _is_visible;
		}

		bool isHovered() const
		{
			return _is_hovered;
		}

		bool hasFocus() const
		{
			return _has_focus;
		}

		void setNextFocus(FocusAction action)
		{
			setNextTakeFocus(action == FocusAction::TakeFocus);
			setNextNoFocus(action == FocusAction::NoFocus);
		}

		void setNextTakeFocus(bool take_focus = true)
		{
			_next_take_focus = take_focus;
		}

		void setNextNoFocus(bool no_focus = true)
		{
			_next_no_focus_on_appearing = no_focus;
		}

		ImGuiWindowFlags& windowFlags()
		{
			return _window_flags;
		}

		ImGuiWindowFlags windowFlags() const
		{
			return _window_flags;
		}

		bool disabledFromCtxStack() const
		{
			return _disable_from_ctx_stack;
		}

		void setDisableFromCtxStack(bool disable = true)
		{
			_disable_from_ctx_stack = disable;
		}

		ImGuiID getDockId() const
		{
			return _dock_id;
		}

		void setDockID(ImGuiID id = 0, bool set_always = false);

		ImGuiID getOrCreateDockId(Context& ctx);

		// 0: not compatible
		virtual u64 getPanelCompatibility(Context& ctx, const Panel* p) const
		{
			return 0;
		}

		//ImGuiDockNode* dockNode() const
		//{
		//	return _dock_node;
		//}
	};
}