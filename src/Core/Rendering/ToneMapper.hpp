#pragma once

#include <Core/Execution/Module.hpp>
#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Execution/Executor.hpp>
#include <Core/IO/GuiContext.hpp>

namespace vkl
{
	class ToneMapper : public Module
	{
	protected:

		MultiDescriptorSetsLayouts _sets_layouts;

		std::string _dst_glsl_format;
		std::shared_ptr<ImageView> _src = nullptr, _dst = nullptr;
		std::shared_ptr<Sampler> _sampler = nullptr;

		std::shared_ptr<ComputeCommand> _compute_tonemap = nullptr;
		struct ComputePC 
		{
			float exposure;
			float gamma;
			float scale;
		};

		bool  _enable = false;
		float _exposure = 1.0f;
		float _log_exposure = 0.0f;
		float _gamma = 1.0f;
		float _scale = 1.0f;
		float _log_scale = 0.0f;

		void createInternalResources();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::shared_ptr<Sampler> sampler = nullptr;
			MultiDescriptorSetsLayouts sets_layouts;
		};
		using CI = CreateInfo;

		ToneMapper(CreateInfo const& ci);

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & exec);

		void declareGui(GuiContext & ctx);
	};
}