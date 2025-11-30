#pragma once

#include <vkl/Execution/Module.hpp>
#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/ComputeCommand.hpp>
#include <vkl/Commands/GraphicsCommand.hpp>

#include <vkl/VkObjects/DetailedVkFormat.hpp>

#include <vkl/Maths/Types.hpp>

#include <vkl/GUI/Context.hpp>

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
		
		std::string _glsl_format;
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

		void declareGui(GUI::Context & ctx);

		void setPosition(Vector2f const& pos)
		{
			_pip_position = pos;
		}

	};
}