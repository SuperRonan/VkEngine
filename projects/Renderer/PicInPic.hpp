#pragma once

#include <Core/Execution/Module.hpp>
#include <Core/Execution/Executor.hpp>

#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/GraphicsCommand.hpp>

#include <Core/VkObjects/DetailedVkFormat.hpp>

#include <Core/Maths/Types.hpp>

#include <Core/IO/GuiContext.hpp>

namespace vkl
{
	class PictureInPicture : public Module
	{
	protected:

		bool _enable = false;
		float _zoom = 8.0;
		float _pip_size = 0.25;
		Vector2f _pip_position = Vector2f(0.5, 0.5);

		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ComputeCommand> _fast_pip = nullptr;

		std::shared_ptr<VertexCommand> _show_outline = nullptr;

		struct FastPiPPC {
			float zoom;
			float pip_size;
			Vector2f pip_pos;
		};

		struct ShowOutlinePC {
			Vector2f pos;
			Vector2f size;
			Vector4f color;
		};

		MultiDescriptorSetsLayouts _sets_layouts;

		Dyn<DefinitionsList> _definitions;

	public:

		struct CreateInfo 
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> target = nullptr;
			MultiDescriptorSetsLayouts sets_layouts;
		};
		using CI = CreateInfo;

		PictureInPicture(CreateInfo const& ci);

		void updateResources(UpdateContext & context);

		void execute(ExecutionRecorder& exec);

		void declareGui(GuiContext & ctx);

		void setPosition(glm::vec2 const& pos)
		{
			_pip_position = pos;
		}

	};
}