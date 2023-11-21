#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/Execution/Module.hpp>
#include <Core/Commands/GraphicsCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>
#include <thatlib/src/img/ImRead.hpp>
#include <Core/IO/ImGuiUtils.hpp>
#include <Core/Execution/Executor.hpp>

namespace vkl
{
	class DebugRenderer : public Module
	{
	protected:

		DefinitionsMap * _common_definitions = nullptr;
		MultiDescriptorSetsLayouts _sets_layouts = {};

		bool _font_loaded = false;
		std::shared_ptr<ImageView> _font = nullptr;
		std::shared_ptr<ImageView> _font_for_upload = nullptr;
		std::shared_ptr<Sampler> _sampler = nullptr;

		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ImageView> _depth = nullptr;
		
		std::shared_ptr<Buffer> _string_buffer = nullptr;

		std::shared_ptr<VertexCommand> _render_strings_with_geometry = nullptr;
		std::shared_ptr<MeshCommand> _render_strings_with_mesh = nullptr;
		std::shared_ptr<FillBuffer> _clear_buffer;

		glm::uvec2 _glyph_size = {16, 16};

		bool _enable_debug = true;
		uint32_t _number_of_debug_strings = 16384;
		int _shader_string_chunks = 8;
		uint32_t _shader_string_capacity = 32;
		int _buffer_string_chunks = 16;
		uint32_t _buffer_string_capacity = 64;

		uint32_t _desc_set = 0;
		uint32_t _first_binding = 0;

		int _default_float_precision = 3;
		ImGuiListSelection _default_glyph_size;
		bool _default_show_plus = false;

		void declareCommonDefinitions();

		void createRenderShader();

		void createResources();

		void loadFont(ExecutionRecorder& exec);

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			DefinitionsMap * common_definitions = nullptr;
			MultiDescriptorSetsLayouts sets_layout = {};
			std::shared_ptr<ImageView> target = nullptr;
			std::shared_ptr<ImageView> depth = nullptr;
		};

		DebugRenderer(CreateInfo const& ci);

		void setTargets(std::shared_ptr<ImageView> const& target, std::shared_ptr<ImageView> const& depth = nullptr);

		void declareImGui();

		void updateResources(UpdateContext & context);

		void execute(ExecutionRecorder & exec);

		ShaderBindings getBindings() const;
	};
}