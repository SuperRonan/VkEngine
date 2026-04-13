#include <vkl/GUI/ObjectInlineInspector.hpp>
#include <vkl/GUI/PanelHolder.hpp>
#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/FancyButtons.hpp>

namespace vkl::GUI
{
	ObjectInlineInspector::ObjectInlineInspector(std::string_view label) :
		_label(label)
	{

	}

	void ObjectInlineInspector::openPanelIFN(Context& ctx, std::shared_ptr<VkObject> const& target, bool set_open)
	{
		PanelHolder* holder = ctx.getTopPanelHolder();
		Panel::Id id = reinterpret_cast<Panel::Id>(target.get());
		if (!_panel && target)
		{
			_panel = holder->getChild(id);
			if (!_panel)
			{
				_panel = target->makeInspector(target, ctx);
				_panel->setOpen(set_open);
				holder->setChild(id, _panel);
			}
		}
		else if (_panel)
		{
			if (set_open)
			{
				_panel->setOpen(set_open);
			}
			if (holder->getChild(id) != _panel || set_open)
			{
				holder->setChild(id, _panel);
			}
		}
	}

	void ObjectInlineInspector::setTargetIFN(GUI::Context& ctx, std::shared_ptr<VkObject> const& target)
	{
		if (this->_target != target.get())
		{
			this->_target = target.get();
			if (target)
			{
				Panel::Id id = reinterpret_cast<Panel::Id>(target.get());
				_panel = ctx.getTopPanelHolder()->getChild(id);
			}
			else
			{
				_panel.reset();
			}
		}
	}

	void ObjectInlineInspector::setAcceptFunction(AcceptObjectFnPtr fn, const void* data)
	{
		_accept_fn = fn;
		_accept_data = data;
	}

	static std::string _collapsing_header_label;

	bool ObjectInlineInspector::declareInline(Context& ctx, std::shared_ptr<VkObject> const& target, std::shared_ptr<VkObject>* dst_target)
	{
		bool res = false;
		setTargetIFN(ctx, target);
		bool declare_inline = false;
		const bool enable_destination = dst_target;

		auto accept_external_source = [&](TransientPayload const& source) -> AcceptObjectResult
			{
				AcceptObjectResult res = {};
				res.accept = enable_destination;
				if (res.accept)
				{
					if (source.object)
					{
						if (_accept_fn)
						{
							res = _accept_fn(_target, source.object.get(), _accept_data);
						}
						else
						{
							res.accept = false;
						}
					}
					else
					{
						res.accept = _accept_nullptr;
						if (!res.accept)
						{
							res.reason = "Cannot accept nullptr";
							res.invalid_type = true; // Can't really know
						}
					}
				}
				else
				{
					res.reason = "Does not accept external sources";
				}
				return res;
			};

		SectionBox box{};
		box.label = _label.c_str();
		box.child_flags |= ImGuiChildFlags_AutoResizeY;

		bool begin_child = box.begin(ctx);

		bool detach = false;
		if (begin_child)
		{
			//ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, std::round(ImGui::GetStyle().ItemSpacing.x * 0.5));
			ImGui::BeginDisabled(!target);
			if (ImGui::DetachButton())
			{
				openPanelIFN(ctx, target, true);
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (target)
			{
				ImGui::BeginDisabled(!_accept_nullptr);
				if (ImGui::TrashButton("Remove"))
				{
					if (dst_target)
					{
						dst_target->reset();
					}
					res = true;
				}
				ImGui::EndDisabled();
			}
			else
			{
				ImGui::BeginDisabled(_disable_create);
				if (ImGui::PlusButton("Create"))
				{
					res = true;
				}
				ImGui::EndDisabled();
			}
			ImGui::SameLine(0, std::round(ImGui::GetStyle().ItemSpacing.x * 1.5));
			//ImGui::PopStyleVar();
			std::string_view object_label = {};
			if (_target)
			{
				object_label = _target->name();
				if (object_label.empty())
				{
					object_label = "Unknown";
				}
			}
			else
			{
				object_label = "Empty (nullptr)";
			}
			_collapsing_header_label.clear();
			std::format_to(std::back_inserter(_collapsing_header_label), "{}###CollpasingHeader", object_label);
			declare_inline = ImGui::CollapsingHeader(_collapsing_header_label.c_str());
			if (ImGui::BeginItemTooltip())
			{
				ImGui::SeparatorIfAppending();
				ImGui::TextUnformatted("A test object");
				ImGui::EndTooltip();
			}

			// Drag And Drop
			{
				auto& payload = ctx.getDragDropPayload();
				if (_enable_source)
				{
					if (ImGui::BeginItemTooltip())
					{
						ImGui::SeparatorIfAppending();
						ImGui::Text("Draggable");
						ImGui::EndTooltip();
					}
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						if (payload.object != target)
						{
							payload.object = target;
						}
						ctx.keepDragDropPayload();
						ImGui::SetDragDropPayload("VkObject", nullptr, 0, ImGuiCond_Once);
						ImGui::Text("%s", object_label.data());
						ImGui::EndDragDropSource();
					}
				}
				if (enable_destination)
				{
					const ImGuiPayload* imgui_payload = ImGui::GetDragDropPayload();
					if (imgui_payload && imgui_payload->IsDataType("VkObject"))
					{
						AcceptObjectResult can_accept = {};
						can_accept = accept_external_source(payload);
						if (ImGui::BeginDragDropTarget())
						{
							if (can_accept.accept && ImGui::AcceptDragDropPayload("VkObject", ImGuiDragDropFlags_None))
							{
								*dst_target = payload.object;
								res = true;
							}
							else if (!can_accept.accept && !can_accept.invalid_type)
							{
								ImGui::CannotAcceptDragDropPayload(can_accept.reason);
							}
							ImGui::EndDragDropTarget();
						}
						else
						{
							if (!can_accept.invalid_type)
							{
								ImGui::SignalDragDropTarget(can_accept.accept);
							}
						}
					}
				}
			}
			auto& clipboard = ctx.getClipboardPayload();
			if (ImGui::BeginPopupContextItem("RMenu", ImGuiPopupFlags_MouseButtonRight))
			{
				bool already_copied = clipboard.object == target;
				if (ImGui::MenuItem("Copy", nullptr, already_copied, _enable_source))
				{
					if (already_copied)
					{
						clipboard.object.reset();
					}
					else
					{
						clipboard.object = target;
					}
				}
				if (!_enable_source)
				{
					ImGui::SetItemTooltip("Cannot copy");
				}
				const AcceptObjectResult can_accept = accept_external_source(clipboard);
				if (ImGui::MenuItem("Paste", nullptr, nullptr, enable_destination && can_accept.accept))
				{
					*dst_target = clipboard.object;
					res = true;
				}
				if (ImGui::BeginItemTooltip())
				{
					const char* payload_label = nullptr;
					if (clipboard.object)
					{
						payload_label = that::GetRawStringPtrIFP(clipboard.object->name());
						if (!payload_label)
						{
							payload_label = "Unknown";
						}
					}
					else
					{
						payload_label = "Empty (nullptr)";
					}
					ImGui::Text("%s", payload_label);
					if (can_accept.reason)
					{
						ImGui::Separator();
						ImGui::Text(can_accept.reason);
					}
					ImGui::EndTooltip();
				}
				if (ImGui::MenuItem("Remove", nullptr, nullptr, (enable_destination && _accept_nullptr && target)))
				{
					dst_target->reset();
					res = true;
				}
				if (ImGui::MenuItem("Create", nullptr, nullptr, (!_disable_create && !target)))
				{
					res = true;
				}
				ImGui::EndPopup();
			}

			if (declare_inline)
			{
				openPanelIFN(ctx, target, false);
				if (_panel)
				{
					_panel->setUsed();
					_panel->declareInline(ctx);
				}
			}
		}

		box.end(ctx);
		if (res)
		{
			if (dst_target)
			{
				if (target.get() == dst_target->get())
				{
					VKL_SHOULD_NOT_BE_HERE;
				}
			}
			else
			{
				VKL_SHOULD_NOT_BE_HERE;
			}
		}
		return res;
	}
}