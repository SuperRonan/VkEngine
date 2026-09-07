#pragma once

#include <vkl/App/VkApplication.hpp>

#include <vkl/Execution/Module.hpp>

#include <vkl/Commands/ComputeCommand.hpp>

#include <vkl/Rendering/Camera.hpp>

#include <that/utils/EnumClassOperators.hpp>

namespace vkl
{
	namespace GUI
	{
		class TemporalAntiAliasingAndUpscalerInspector;
	}
	class TemporalAntiAliasingAndUpscaler : public Module
	{
	public:
		enum class Mode
		{
			Default
		};

		struct Requirements
		{
			enum class Flags : u8
			{
				None = 0x0,
				Checkerboard = 0x1,
			};
			enum class MotionFlags : u8
			{
				None = 0x0,
				F16 = 0x1,
				F32 = 0x2,
			};
			Flags flags = Flags::None;
			MotionFlags motion = MotionFlags::None;
			u8 image_memory = 0; // Number of extra layers of previous input images needed (0 means 1 layer (current frame) is needed)
			Vector2u input_resolution = {};
			Vector2f downscale = {};
		};

		enum class Input : u32
		{
			Color = 0,
			_Count,
		};

	protected:

		static const constexpr float _Default_Renew_Rate = rcp(float(1024));

		std::shared_ptr<ComputeCommand> _temporal_intergration;
		std::shared_ptr<ImageView> _output;

		MyVector<std::shared_ptr<ImageView>> _inputs = {};

		bool _enable = true;
		bool _reset = true;

		Mode _mode = Mode::Default;
		float _renew_rate = _Default_Renew_Rate;
		Vector2u _downsample_integral = Vector2u(1, 1);

		uint32_t _accumulated_samples = 0;

		Dyn<VkFormat> _accumation_format = {};
		std::string _format_glsl;

		MultiDescriptorSetsLayouts _sets_layouts;

		//const Requirements* _p_renderer_available_requirements = {};

		std::shared_ptr<ComputeCommand> _taau_command;
		struct TAAU_PushConstant
		{
			float new_sample_weight;
			uint32_t flags;
		};

		void setFormat();

		Matrix4f _matrix;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			VkImageType image_type = VK_IMAGE_TYPE_2D;
			Dyn<VkExtent3D> extent = {}; // Mandatory
			Dyn<u32> layers = {}; // Optional
			MultiDescriptorSetsLayouts sets_layouts;
			//const Requirements* p_renderer_available_requirements = {};
		};
		using CI = CreateInfo;

		TemporalAntiAliasingAndUpscaler(CreateInfo const& ci);

		void setInput(Input input_id, std::shared_ptr<ImageView> const& img);
		std::shared_ptr<ImageView> const& getInput(Input input_id) const;

		virtual ~TemporalAntiAliasingAndUpscaler() override;

		void updateResources(UpdateContext& ctx);

		void execute(ExecutionRecorder& recorder, Camera const& camera);

		using InspectorType = GUI::TemporalAntiAliasingAndUpscalerInspector;
		friend class InspectorType;
		virtual std::shared_ptr<GUI::Panel> makeInspector(GUI::InspectorMakeInfo const& imi);

		std::shared_ptr<ImageView> const& output()const
		{
			return _output;
		}

		Requirements calcFrameRequirements();
	};
}

THAT_DECLARE_ENUM_CLASS_OPERATORS(vkl::TemporalAntiAliasingAndUpscaler::Requirements::Flags);
THAT_DECLARE_ENUM_CLASS_OPERATORS(vkl::TemporalAntiAliasingAndUpscaler::Requirements::MotionFlags);