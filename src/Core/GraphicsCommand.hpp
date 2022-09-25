#pragma once

#include "ShaderCommand.hpp"

namespace vkl
{
	class GraphicsCommand : public ShaderCommand
	{
	protected:

		std::vector<std::shared_ptr<ImageView>> _attachements;
		std::shared_ptr<RenderPass> _render_pass;
		std::shared_ptr<Framebuffer> _framebuffer;

		virtual void createGraphicsResources();

	public:

		GraphicsCommand(VkApplication* app, std::string const& name, std::vector<ShaderBindingDescriptor> const& bindings, std::vector<std::shared_ptr<ImageView>> const& targets);

		virtual void init() override;

		virtual void declareGraphicsResources();

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) = 0;

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context) override;

		virtual void execute(ExecutionContext& context) override;
	};

	// Uses the "classic" vertex pipeline
	class VertexCommand : public GraphicsCommand
	{
	protected:

		
		std::vector<std::shared_ptr<Mesh>> _meshes;
		std::shared_ptr<ImageView> _depth_stencil;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::optional<uint32_t> draw_size;
			std::vector<ShaderBindingDescriptor> bindings = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::filesystem::path vertex_shader_path = {};
			std::filesystem::path geometry_shader_path = {};
			std::filesystem::path fragment_shader_path = {};
			std::vector<std::string> definitions;
		};

		using CI = CreateInfo;

		VertexCommand(CreateInfo const& ci);

		virtual void init() override;

		virtual void declareGraphicsResources() override;

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

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) override;

	};
}