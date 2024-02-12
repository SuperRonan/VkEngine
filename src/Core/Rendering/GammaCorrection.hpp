#pragma once

#include <Core/Execution/Module.hpp>
#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Execution/Executor.hpp>
#include <Core/IO/GuiContext.hpp>
#include <Core/VkObjects/Swapchain.hpp>

namespace vkl
{
	class GammaCorrection : public Module
	{
	protected:

		MultiDescriptorSetsLayouts _sets_layouts;

		std::string _dst_glsl_format;
		std::shared_ptr<ImageView> _src = nullptr, _dst = nullptr;
		std::shared_ptr<Sampler> _sampler = nullptr;

		std::shared_ptr<ComputeCommand> _compute_tonemap = nullptr;

		std::shared_ptr<Swapchain> _swapchain = nullptr;
		bool _auto_fit_to_swapchain = true;
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

		struct ComputePC 
		{
			float exposure;
			float gamma;
		};

		bool  _enable = false;
		float _exposure = 1.0f;
		float _log_exposure = 0.0f;
		float _gamma = 1.0f;

		size_t _plot_samples = 100;
		size_t _plot_min_radiance = 0;
		size_t _plot_max_radiance = 1;
		std::vector<float> _plot_raw_radiance;
		std::vector<float> _plot_gamma_radiance;

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
			std::shared_ptr<Swapchain> swapchain = nullptr;
		};
		using CI = CreateInfo;

		GammaCorrection(CreateInfo const& ci);

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & exec);

		void declareGui(GuiContext & ctx);

		float computeGammaCorrection(float f)const;
	};
}