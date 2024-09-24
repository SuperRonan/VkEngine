#pragma once

#include <vkl/Commands/GraphicsCommand.hpp>
#include <vkl/Execution/Module.hpp>
#include <vkl/IO/GuiContext.hpp>

#include <vkl/Rendering/Camera.hpp>

#include <bitset>

namespace vkl
{

	class RenderWorldBasis : public Module
	{
	public:

	protected:

		MultiDescriptorSetsLayouts _sets_layouts = {};
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<Framebuffer> _framebuffer = nullptr;
		uint32_t _subpass_index = 0;

		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ImageView> _depth = nullptr;

		AttachmentBlending _blending;

		std::shared_ptr<VertexCommand> _render_planes = nullptr;
		std::shared_ptr<VertexCommand> _render_lines = nullptr;

		uint32_t _flags = (0b111u << 29u) | 10;

		Dyn<bool> _render_in_log_space = {};

		void createInternals();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts;
			std::shared_ptr<RenderPass> extern_render_pass = nullptr;
			uint32_t subpass_index = 0;
			std::shared_ptr<ImageView> target = nullptr;
			std::shared_ptr<ImageView> depth = nullptr;
			Dyn<bool> render_in_log_space = {};
		};
		using CI = CreateInfo;


		RenderWorldBasis(CreateInfo const& ci);

		virtual ~RenderWorldBasis() override;

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & exec, Camera & camera);

		void declareGUI(GuiContext & ctx);
	};

}