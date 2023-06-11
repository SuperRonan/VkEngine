#pragma once

#include <Core/VkApplication.hpp>
#include <Core/ImageView.hpp>
#include <Core/Module.hpp>
#include <Core/GraphicsCommand.hpp>
#include <Core/TransferCommand.hpp>
#include <Core/Executor.hpp>

namespace vkl
{
	class DebugRenderer : public Module
	{
	protected:

		Executor& _exec;
		
		std::shared_ptr<ImageView> _font = nullptr;
		std::shared_ptr<ImageView> _target = nullptr;
		
		std::shared_ptr<Buffer> _string_buffer = nullptr;

		std::shared_ptr<VertexCommand> _render_strings = nullptr;
		std::shared_ptr<FillBuffer> _clear_buffer;

		bool _enable_debug = true;
		uint32_t _number_of_debug_strings = 16384;
		uint32_t _shader_string_capacity = 32;
		uint32_t _buffer_string_capacity = 64;

		uint32_t _desc_set = 0;
		uint32_t _first_binding = 0;

	public:

		struct CreateInfo
		{
			Executor& exec;
			std::shared_ptr<ImageView> target;
		};

		DebugRenderer(CreateInfo const& ci);

		void declareToImGui();

		void execute();

	};
}