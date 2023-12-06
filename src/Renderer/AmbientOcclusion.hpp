#pragma once

#include <Core/Execution/Module.hpp>
#include <Core/Execution/Executor.hpp>

#include <Core/Commands/ComputeCommand.hpp>

#include <Core/IO/GuiContext.hpp>

#include <Core/Rendering/Camera.hpp>

namespace vkl
{
	class AmbientOcclusion : public Module
	{
	public:
		
		struct UBO
		{
			float radius;
		};

	protected:

		bool _enable = true;

		float _radius = 0.02;

		MultiDescriptorSetsLayouts _sets_layouts = {};

		std::shared_ptr<ComputeCommand> _command = nullptr;

		struct CommandPC
		{
			glm::vec3 camera_position;
			uint32_t flags;
			float radius;
		};

		std::shared_ptr<Sampler> _sampler = {};

		std::shared_ptr<ImageView> _positions = nullptr;
		std::shared_ptr<ImageView> _normals = nullptr;

		VkFormat _format;
		std::string _format_glsl = {};

		std::shared_ptr<ImageView> _target = nullptr;

		void createInternalResources();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::shared_ptr<ImageView> positions = nullptr;
			std::shared_ptr<ImageView> normals = nullptr;
		};
		using CI = CreateInfo;

		AmbientOcclusion(CreateInfo const& ci);

		
		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & recorder, const Camera & camera);

		void declareGui(GuiContext & ctx);


		const std::shared_ptr<ImageView>& target()const
		{
			return _target;
		}

		bool enable()const
		{
			return _enable;
		}
	};
}