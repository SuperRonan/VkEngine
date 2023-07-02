#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/Execution/Module.hpp>
#include <Core/Commands/GraphicsCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>
#include <Core/Execution/Executor.hpp>
#include <thatlib/src/img/ImRead.hpp>
#include <Core/IO/ImGuiUtils.hpp>

namespace vkl
{
	class DebugRenderer : public Module
	{
	protected:

		Executor& _exec;
		
		bool _font_loaded = false;
		std::shared_ptr<ImageView> _font = nullptr;
		std::shared_ptr<ImageView> _font_for_upload = nullptr;
		std::shared_ptr<Sampler> _sampler = nullptr;

		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ImageView> _depth = nullptr;
		
		std::shared_ptr<Buffer> _string_buffer = nullptr;

		std::shared_ptr<VertexCommand> _render_strings = nullptr;
		std::shared_ptr<FillBuffer> _clear_buffer;


		bool _enable_debug = true;
		uint32_t _number_of_debug_strings = 16384;
		int _shader_string_chunks = 8;
		uint32_t _shader_string_capacity = 32;
		int _buffer_string_chunks = 8;
		uint32_t _buffer_string_capacity = 64;

		uint32_t _desc_set = 0;
		uint32_t _first_binding = 0;

		int _default_float_precision = 3;
		ImGuiRadioButtons _default_glyph_size;
		bool _default_show_plus = false;

	public:

		struct CreateInfo
		{
			Executor& exec;
			std::shared_ptr<ImageView> target;
			std::shared_ptr<ImageView> depth;
		};

		DebugRenderer(CreateInfo const& ci);

		void declareToImGui();

		void loadFont();

		void execute();

	};
}