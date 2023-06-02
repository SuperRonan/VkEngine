#pragma once

#include "ShaderCommand.hpp"

namespace vkl
{
	class GraphicsCommand : public ShaderCommand
	{
	protected:

		std::shared_ptr<GraphicsProgram> _program;
		std::vector<std::shared_ptr<ImageView>> _attachements = {};
		std::shared_ptr<ImageView> _depth = nullptr;
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<Framebuffer> _framebuffer = nullptr;

		std::optional<bool> _write_depth = {};

		std::optional<VkClearColorValue> _clear_color = {};
		std::optional<VkClearDepthStencilValue> _clear_depth_stencil = {};

		std::optional<VkPipelineColorBlendAttachmentState> _blending = {};

		virtual void createProgramIFN() = 0;

		virtual void createGraphicsResources();

		void createPipeline();

		virtual void declareGraphicsResources(InputSynchronizationHelper & synch);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::vector<std::shared_ptr<ImageView>> targets = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::optional<bool> write_depth = {};
			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};
			std::optional<VkPipelineColorBlendAttachmentState> blending = {};
		};

		GraphicsCommand(CreateInfo const& ci);

		virtual void init() override;
		
		struct DrawInfo
		{
			PushConstant pc;
			VkViewport viewport;
		};

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context, DrawInfo const& di, void * user_data);

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context, void* user_data) = 0;

		virtual bool updateResources(UpdateContext & ctx) override;
	};




	// Uses the "classic" vertex pipeline
	class VertexCommand : public GraphicsCommand
	{
	protected:

		struct ShaderPaths
		{
			std::filesystem::path vertex_path;
			std::filesystem::path geometry_path;
			std::filesystem::path fragment_path;
			DynamicValue<std::vector<std::string>> definitions;
		};

		ShaderPaths _shaders;

		std::vector<std::shared_ptr<Mesh>> _meshes;

		DynamicValue<uint32_t> _draw_count = false;



		virtual void createProgramIFN() override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			DynamicValue<uint32_t> draw_count = {};
			std::vector<std::shared_ptr<Mesh>> meshes = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::optional<bool> write_depth = {};
			std::filesystem::path vertex_shader_path = {};
			std::filesystem::path geometry_shader_path = {};
			std::filesystem::path fragment_shader_path = {};
			DynamicValue<std::vector<std::string>> definitions = {};
			
			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};

			std::optional<VkPipelineColorBlendAttachmentState> blending = {};
		};
		using CI = CreateInfo;

		struct DrawInfo
		{
			PushConstant pc = {};
			uint32_t draw_count = 0;
			std::optional<VkViewport> viewport = {};
		};
		using DI = DrawInfo;

		VertexCommand(CreateInfo const& ci);

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context, void * user_data) override;

		void execute(ExecutionContext& ctx, DrawInfo const& di);

		virtual void execute(ExecutionContext& ctx) override;

		virtual bool updateResources(UpdateContext & ctx) override;

		Executable with(DrawInfo const& di);
		
		Executable operator()(DrawInfo const& di)
		{
			return with(di);
		}
	};

	// Uses the mesh pipeline
	class MeshCommand : public GraphicsCommand
	{
		// TODO
	};

	class FragCommand : public GraphicsCommand
	{
	protected:

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::filesystem::path fragment_shader_path = {};
			DynamicValue<std::vector<std::string>> definitions;
		};

		using CI = CreateInfo;

		FragCommand(CreateInfo const& ci);

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context);

	};
}