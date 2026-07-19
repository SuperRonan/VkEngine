#define IMGUI_DEFINE_MATH_OPERATORS 1
#include <vkl/GUI/Context.hpp>

#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Sampler.hpp>
#include <vkl/VkObjects/DescriptorSetLayout.hpp>

#include <imgui/backends/imgui_impl_vulkan.h>

namespace vkl::GUI
{
	std::shared_ptr<Style> g_default_style = [](){
		using Color = Style::Color;
		Color default_border_color = Color(0.50f, 0.50f, 0.50f, 0.50f);
		std::shared_ptr<Style> res = std::make_shared<Style>(Style{
			.valid_green = Color(0, 0.8, 0, 1),
			.invalid_red = Color(0.8, 0, 0, 1),
			.warning_yellow = Color(0.8, 0.8, 0, 1),
			.stack_colors = {
				default_border_color,
				default_border_color * ImVec4(1.5, 0.5, 0.5, 1),
				default_border_color * ImVec4(0.5, 1.5, 0.5, 1),
				default_border_color * ImVec4(0.5, 0.5, 1.5, 1),
			},
		});
		return res;
	}();

	const char* GetEnumStyleShortStr(EnumStyle es)
	{
		const char* res = nullptr;
		
		switch (es)
		{
			case EnumStyle::Label:
				res = "STR";
			break;
			case EnumStyle::Decimal:
				res = "123";
			break;
			case EnumStyle::Hexa:
				res = "0xf";
			break;
			//case EnumStyle::Binary:
			//	res = "0b1";
			//break;
		}
		return res;
	}

	EnumStyle CycleNextEnumStyle(EnumStyle es)
	{
		return static_cast<EnumStyle>(
			(static_cast<int>(es) + 1) % (static_cast<int>(EnumStyle::MAX_VALUE) + 1)
		);
	}

	bool DeclareEnumStyleButtonSwitch(EnumStyle* p_style)
	{
		bool res = false;
		EnumStyle style = p_style ? *p_style : EnumStyle::Default;
		const char* style_str = GetEnumStyleShortStr(style);
		ImGui::BeginDisabled(!p_style);
		if (ImGui::SmallButton(style_str))
		{
			if (p_style)
			{
				*p_style = CycleNextEnumStyle(*p_style);
				res = true;
			}
		}
		if (ImGui::BeginItemTooltip())
		{
			ImGui::Text("Switch enum value preview.");
			ImGui::EndTooltip();
		}
		ImGui::EndDisabled();
		return res;
	}

	Context::Context(CreateInfo const& ci) :
		_imgui_context(ci.imgui_context),
		_style(ci.style ? ci.style : g_default_style),
		_common_file_dialog(ci.common_file_dialog)
	{
		if (!_common_file_dialog)
		{
			_common_file_dialog = std::make_shared<FileDialog>(FileDialog::CI{

			});
		}
	}

	void Context::createInternalResource(std::shared_ptr<DescriptorSetLayoutInstance> const& texture_set_layout, std::shared_ptr<DescriptorSetLayoutInstance> const& sampler_set_layout)
	{
		_imgui_texture_set_layout = texture_set_layout;
		_imgui_sampler_set_layout = sampler_set_layout;
		VkApplication* app = _imgui_texture_set_layout->application();
		_sampler = Sampler::MakeNearest(app);
		_sampler->setName("ImGui Default Sampler");
	}

	void Context::begin()
	{
		if (!_keep_drag_drop_payload)
		{
			_drag_drop_payload.object.reset();
			_keep_drag_drop_payload = false;
		}

		ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
		_imgui_bound_sampler = {}; // Set to nullptr <=> the default imgui sampler is used
		_imgui_bound_image = {}; // Set to nullptr <=> the default imgui texture is used
	}

	void Context::end()
	{
		++_frame_counter;
	}

	void Context::pushPanelHolder(PanelHolder* panel)
	{
		_panel_holder_stack.push_back(panel);
	}

	void Context::popPanelHolder()
	{
		_panel_holder_stack.pop_back();
	}

	PanelHolder* Context::getTopPanelHolder(uint index) const
	{
		return _panel_holder_stack.at(_panel_holder_stack.size32() - index - 1);
	}

	PanelHolder* Context::getBottomPanelHolder(uint index) const
	{
		return _panel_holder_stack.at(index);
	}

	void Context::pushPanel(Panel* panel)
	{
		_panel_stack.push_back(panel);
	}

	void Context::popPanel()
	{
		_panel_stack.pop_back();
	}

	Panel* Context::getTopPanel(uint index) const
	{
		return _panel_stack.at(_panel_stack.size32() - index - 1);
	}

	Panel* Context::getBottomPanel(uint index) const
	{
		return _panel_stack.at(index);
	}

	std::span<PanelHolder* const> Context::getPanelHolderStack() const
	{
		return std::span<PanelHolder* const>(_panel_holder_stack.data(), _panel_holder_stack.size());
	}

	std::span<Panel* const> Context::getPanelStack() const
	{
		return std::span<Panel* const>(_panel_stack.data(), _panel_stack.size());
	}

	Style::Color Context::pushStack()
	{
		const auto& colors = style().stack_colors;
		if (colors.empty())
		{
			++_stack_counter;
			return ImGui::GetStyleColorVec4(ImGuiCol_Border);
		}
		Style::Color res = colors[_stack_counter % static_cast<uint32_t>(colors.size())];
		++_stack_counter;
		return res;
	}

	Style::Color Context::popStack()
	{
		--_stack_counter;
		const auto& colors = style().stack_colors;
		if (colors.empty())
		{
			return ImGui::GetStyleColorVec4(ImGuiCol_Border);
		}
		Style::Color res = colors[_stack_counter % static_cast<uint32_t>(colors.size())];
		return res;
	}

	void Context::clearTemporaryData()
	{
		_clipboard_payload.object.reset();
	}
}