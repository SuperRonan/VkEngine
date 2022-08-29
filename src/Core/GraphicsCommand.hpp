#pragma once

#include "ShaderCommand.hpp"

namespace vkl
{
	class GraphicsCommand : public ShaderCommand
	{
	protected:

		std::shared_ptr<RenderPass> _render_pass;
		std::shared_ptr<Framebuffer> _framebuffer;

		// Here ?
		std::shared_ptr<Buffer> _mesh_buffer;

	public:

		virtual void init() override;

		virtual void declareGraphicsResources();

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) = 0;

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context) override;

		virtual void execute(ExecutionContext& context) override;
	};

	class DrawMeshCommand : public GraphicsCommand
	{
	protected:

		std::vector<std::shared_ptr<Mesh>> _meshes;

	public:

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) override;
	};

	class FragCommand : public GraphicsCommand
	{
	protected:

	public:

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) override;

	};
}