#pragma once

#include <Core/App/VkApplication.hpp>

#include <Core/Execution/Module.hpp>

#include <Core/Commands/ComputeCommand.hpp>

#include <Core/IO/GuiContext.hpp>

#include <Core/Rendering/Camera.hpp>

namespace vkl
{
	class TemporalAntiAliasingAndUpscaler : public Module
	{
	protected:

		std::shared_ptr<ComputeCommand> _temporal_intergration;
		std::shared_ptr<ImageView> _input;
		std::shared_ptr<ImageView> _output;

		bool _enable = true;
		float _alpha = 0.9;

		std::string _format_glsl;

		MultiDescriptorSetsLayouts _sets_layouts;

		std::shared_ptr<ComputeCommand> _taau_command;
		struct TAAU_PushConstant
		{
			float alpha;
			uint32_t flags;
		};

		void setFormat();

		Matrix4f _matrix;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> input = nullptr;
			MultiDescriptorSetsLayouts sets_layouts;
		};
		using CI = CreateInfo;

		TemporalAntiAliasingAndUpscaler(CreateInfo const& ci);

		void updateResources(UpdateContext& ctx);

		void execute(ExecutionRecorder& recorder, Camera const& camera);

		void declareGui(GuiContext& ctx);

		std::shared_ptr<ImageView> const& output()const
		{
			return _output;
		}
	};
}