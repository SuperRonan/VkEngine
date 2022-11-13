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

		virtual void createProgramIFN() = 0;

		virtual void createGraphicsResources();

		virtual void declareGraphicsResources();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<ShaderBindingDescriptor> bindings = {};
			std::vector<std::shared_ptr<ImageView>> targets = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::optional<bool> write_depth = {};
			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};
		};

		GraphicsCommand(CreateInfo const& ci);

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) = 0;

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context) override;

		virtual void execute(ExecutionContext& context) override;
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
			std::vector<std::string> definitions;
		};

		ShaderPaths _shaders;

		std::vector<std::shared_ptr<Mesh>> _meshes;

		std::optional<uint32_t> _draw_count = false;

		std::optional<VkPipelineColorBlendAttachmentState> _blending = {};

		virtual void createProgramIFN() override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::optional<uint32_t> draw_count = {};
			std::vector<std::shared_ptr<Mesh>> meshes = {};
			std::vector<ShaderBindingDescriptor> bindings = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::optional<bool> write_depth = {};
			std::filesystem::path vertex_shader_path = {};
			std::filesystem::path geometry_shader_path = {};
			std::filesystem::path fragment_shader_path = {};
			std::vector<std::string> definitions = {};
			
			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};

			std::optional<VkPipelineColorBlendAttachmentState> blending = {};
		};

		using CI = CreateInfo;

		VertexCommand(CreateInfo const& ci);

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) override;
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
			std::vector<ShaderBindingDescriptor> bindings = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::filesystem::path fragment_shader_path = {};
			std::vector<std::string> definitions;
		};

		using CI = CreateInfo;

		FragCommand(CreateInfo const& ci);

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) override;

	};
}