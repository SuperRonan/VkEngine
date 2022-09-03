#pragma once

#include "ShaderCommand.hpp"

namespace vkl
{
	class GraphicsCommand : public ShaderCommand
	{
	protected:

		std::shared_ptr<RenderPass> _render_pass;
		std::shared_ptr<Framebuffer> _framebuffer;

	public:

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