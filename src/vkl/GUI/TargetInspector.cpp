#include <vkl/GUI/TargetInspector.hpp>

#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>

#include <typeinfo>

namespace vkl::GUI
{
	TargetInspectorBase::TargetInspectorBase(std::shared_ptr<VkObject> const& target) :
		Panel(target->application(), std::format("{}###{}", target->name(), reinterpret_cast<uintptr_t>(target.get()))),
		_target(target)
	{
		_window_flags |= ImGuiWindowFlags_MenuBar;
	}

	TargetInspectorBase::TargetInspectorBase(std::shared_ptr<VkObject> const& target, std::string_view class_type) :
		Panel(target->application(), std::format("{} - {} Inspector###{}", target->name(), class_type, reinterpret_cast<uintptr_t>(target.get()))),
		_target(target)
	{
		_window_flags |= ImGuiWindowFlags_MenuBar;
	}

	void TargetInspectorBase::declareMenu(Context& ctx)
	{
		if (ImGui::BeginMenu("Object"))
		{
			auto& clipboard = ctx.getClipboardPayload();
			bool already_copied = clipboard.object == _target;
			if (ImGui::MenuItem("Copy", nullptr, already_copied, _enable_source))
			{
				if (already_copied)
				{
					clipboard.object.reset();
				}
				else
				{
					clipboard.object = _target;
				}
			}
			ImGui::EndMenu();
		}
	}

	void TargetInspectorBase::declareInline(Context& ctx)
	{
		if (ctx.getTopPanel() == this)
		{

		}

		if (_target)
		{
			ImGui::LabelText2("Name", _target->name().c_str());
		}
		else
		{
			ImGui::Text("Empty (nullptr)");
		}
		ImGui::LabelValue("Ref Count", _target.use_count());
		ImGui::SetItemTooltip("This GUI panel holds 1 ref.");
		const char* type_name = nullptr;
#if VKL_HAS_STD_RTTI
		if (_target)
		{
			type_name = typeid(*_target).name();
		}
#endif
		if (!type_name)
		{
			type_name = "Unknown";
		}
		ImGui::LabelText2("Type", type_name);
	}

	void TargetInspectorBase::DeclareClass(Context& ctx, std::type_info const& type_info)
	{
		const char* type_name = type_info.name();
		if (!type_name)
		{
			type_name = "Unknown";
		}
		ImGui::LabelText2("Class", type_name);
	}
}