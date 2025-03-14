#pragma once

#include <vkl/App/VkApplication.hpp>

#include <vkl/VkObjects/ImageView.hpp>


#include <vkl/Commands/GraphicsCommand.hpp>
#include <vkl/Commands/TransferCommand.hpp>

#include <vkl/IO/ImGuiUtils.hpp>
#include <vkl/IO/GuiContext.hpp>

#include <vkl/Execution/Module.hpp>
#include <vkl/Execution/Executor.hpp>
#include <vkl/Rendering/TextureFromFile.hpp>

namespace vkl
{
	class DebugRenderer : public Module
	{
	protected:

		DefinitionsMap * _common_definitions = nullptr;
		MultiDescriptorSetsLayouts _sets_layouts = {};

		std::shared_ptr<TextureFromFile> _font = nullptr;
		std::shared_ptr<Sampler> _sampler = nullptr;

		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ImageView> _depth = nullptr;
		
		std::shared_ptr<Buffer> _debug_buffer = nullptr;
		BufferSegment _debug_buffer_header;
		BufferSegment _debug_buffer_strings_meta;
		BufferSegment _debug_buffer_strings_content;
		BufferSegment _debug_buffer_lines;

		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<Framebuffer> _framebuffer = nullptr;

		AttachmentBlending _blending = {};

		std::shared_ptr<VertexCommand> _render_strings_with_geometry = nullptr;
		std::shared_ptr<VertexCommand> _render_lines_with_geometry = nullptr;
		std::shared_ptr<MeshCommand> _render_strings_with_mesh = nullptr;
		std::shared_ptr<MeshCommand> _render_lines_with_mesh = nullptr;

		// Texture glyph size
		Vector2u _glyph_size = {16, 16};

		bool _enable_debug = true;
		int _shader_string_chunks = 8;
		uint32_t _shader_string_capacity = 32;
		bool _define_capacity = false;

		uint32_t _log2_number_of_debug_strings = 10;
		uint32_t _number_of_debug_strings;
		uint32_t _log2_debug_chunks = 16; // in Bytes
		uint32_t _debug_chunks_capacity; // in Bytes
		uint32_t _log2_number_of_debug_lines = 14;
		uint32_t _number_of_debug_lines;

		uint32_t _fixed_header_size;
		uint32_t _atomic_counters_size;

		size_t _debug_buffer_size;

		uint32_t _desc_set = 0;
		uint32_t _first_binding = 0;

		int _default_float_precision = 3;
		// In shader display font size
		ImGuiListSelection _default_glyph_size;
		bool _default_show_plus = false;

		bool _should_write_header = true;

		void declareCommonDefinitions();

		void createRenderShader();

		void createResources();

		//void loadFont(ExecutionRecorder& exec);

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

		virtual ~DebugRenderer() override;

		void setTargets(std::shared_ptr<ImageView> const& target, std::shared_ptr<ImageView> const& depth = nullptr);

		void declareGui(GuiContext & ctx);

		void updateResources(UpdateContext & context);

		void execute(ExecutionRecorder & exec);

		ShaderBindings getBindings() const;

		static MyVector<DescriptorSetLayout::Binding> getLayoutBindings(uint32_t offset);
	};
}