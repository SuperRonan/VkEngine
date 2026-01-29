#pragma once

#include <vkl/Execution/Module.hpp>

#include <vkl/Commands/ComputeCommand.hpp>

#include <vkl/Rendering/Camera.hpp>

namespace vkl
{
	namespace GUI
	{
		class DepthOfFieldInspector;
	}
	class DepthOfField : public Module
	{
	protected:

		std::shared_ptr<ImageView> _target;

		std::shared_ptr<ImageView> _target_copy;
		std::shared_ptr<Sampler> _sampler;

		std::shared_ptr<ImageView> _depth;
		std::shared_ptr<Sampler> _depth_sampler;

		std::shared_ptr<ComputeCommand> _command;

		const Camera* _camera = nullptr;

		MultiDescriptorSetsLayouts _sets_layouts = {};

		void createInternals();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> target = nullptr;
			std::shared_ptr<ImageView> depth = nullptr;
			const Camera* camera = nullptr;
			MultiDescriptorSetsLayouts sets_layouts = {};
		};
		using CI = CreateInfo;

		DepthOfField(CreateInfo const& ci);

		virtual ~DepthOfField() override = default;

		virtual void updateResources(UpdateContext& ctx);

		virtual void record(ExecutionRecorder& exec);

		using InspectorType = GUI::DepthOfFieldInspector;
		friend class InspectorType;
		virtual std::shared_ptr<GUI::Panel> makeInspector(std::shared_ptr<DepthOfField> const& shared_this, GUI::Context& ctx);
	};
}