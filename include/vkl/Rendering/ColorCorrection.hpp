#pragma once

#include <vkl/Execution/Module.hpp>
#include <vkl/Commands/ComputeCommand.hpp>
#include <vkl/Execution/Executor.hpp>

#include <vkl/VkObjects/VkWindow.hpp>

namespace vkl
{
	namespace GUI
	{
		class ColorCorrectionInspector;
	}

	class ColorCorrection : public Module
	{
	public:

	protected:

		MultiDescriptorSetsLayouts _sets_layouts;

		std::string _dst_glsl_format;
		std::shared_ptr<ImageView> _src = nullptr, _dst = nullptr;
		std::shared_ptr<Sampler> _sampler = nullptr;

		bool _show_test_card = false;
		std::shared_ptr<ComputeCommand> _render_test_card = nullptr;

		std::shared_ptr<ComputeCommand> _compute_tonemap = nullptr;

		std::shared_ptr<VkWindow> _target_window = nullptr;
		bool _auto_fit_to_window = true;
		struct SwapchainInfo 
		{
			VkSurfaceFormatKHR format = VkSurfaceFormatKHR{
				.format = VK_FORMAT_MAX_ENUM,
				.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR,
			};
			//constexpr std::strong_ordering operator<=>(SwapchainInfo const&) const = default;
			constexpr bool operator!=(SwapchainInfo const& other) const
			{
				return format != other.format;
			}

			constexpr bool operator==(SwapchainInfo const& other) const
			{
				return format == other.format;
			}
		};
		SwapchainInfo _prev_swapchain_info = {};

		ColorCorrectionMode _mode = ColorCorrectionMode::PassThrough;
		float _exposure = 1.0f;
		float _gamma = 1.0f;

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
			std::shared_ptr<VkWindow> target_window = nullptr;
		};
		using CI = CreateInfo;

		ColorCorrection(CreateInfo const& ci);

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & exec);

		using InspectorType = GUI::ColorCorrectionInspector;
		friend class InspectorType;
		virtual std::shared_ptr<GUI::Panel> makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx) override;

		float computeTransferFunction(float linear)const;
	};
}