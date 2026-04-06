#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <imgui/imgui.h>

namespace vkl::GUI
{
	class PopUp
	{
	public:

		enum class Result
		{
			Pending = 0,
			Cancel = -1,
			Create = 1,
		};

		enum class Style : uint8_t
		{
			Modal = 0,
			Classic = 1,
		};

	protected:

		std::string _name = {};
		ImGuiPopupFlags _flags = ImGuiPopupFlags_None;
		ImGuiWindowFlags _window_flags = ImGuiWindowFlags_None;
		bool _open = false;
		bool _keep_open_on_create = false;
		Style _style = Style::Modal;

	public:

		PopUp(std::string_view name) :
			_name(name)
		{

		}

		std::string const& name() const
		{
			return _name;
		}

		void setName(std::string_view const& new_name)
		{
			_name = new_name;
		}

		void open(Context& ctx);

		void close();

		bool isOpen() const
		{
			return _open;
		}

		virtual Result declare(Context& ctx);

		virtual Result declareInline(Context& ctx) = 0;

		static Result DeclareResultButtons(Context& ctx, bool enable_creation = true, const char* create_label = nullptr);
	};
}