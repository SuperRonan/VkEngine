#pragma once

#include <vkl/App/VkApplication.hpp>

#include <vkl/Execution/Module.hpp>

#include <vkl/Commands/ComputeCommand.hpp>

#include <vkl/GUI/Context.hpp>

#include <vkl/Rendering/Camera.hpp>

namespace vkl
{
	class TemporalAntiAliasingAndUpscaler : public Module
	{
	protected:

		std::shared_ptr<ComputeCommand> _temporal_intergration;
		std::shared_ptr<ImageView> _input;
		std::shared_ptr<ImageView> _output;

		bool _enable = true;
		ImGuiListSelection _mode;
		float _alpha = 0.9;
		uint _max_samples = 128*128;

		uint32_t _accumulated_samples = 0;
		bool _reset = true;


		ImGuiListSelection _gui_acc_format;
		Dyn<VkFormat> _accumation_format = {};
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

		virtual ~TemporalAntiAliasingAndUpscaler() override;

		void updateResources(UpdateContext& ctx);

		void execute(ExecutionRecorder& recorder, Camera const& camera);

		void declareGui(GUI::Context& ctx);

		std::shared_ptr<ImageView> const& output()const
		{
			return _output;
		}
	};
}