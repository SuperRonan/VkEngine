#pragma once

#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Sampler.hpp>

#include <vkl/Execution/Module.hpp>
#include <vkl/Execution/Executor.hpp>

#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>

#include <vkl/Commands/TransferCommand.hpp>
#include <vkl/Commands/GraphicsTransferCommands.hpp>
#include <vkl/Commands/ComputeCommand.hpp>



namespace vkl
{
	class ImagePicker : public Module
	{
	protected:

		MyVector<std::shared_ptr<ImageView>> _sources = {};

		ImGuiListSelection _gui_source;

		std::shared_ptr<ImageView> _dst = nullptr;

		std::shared_ptr<BlitImage> _blitter = nullptr;

		ImGuiListSelection _gui_filter;

		VkFilter _filter = VK_FILTER_NEAREST;

		bool _latest_success = true;

	public:
		
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MyVector<std::shared_ptr<ImageView>> sources = {};
			size_t index = 0;
			std::shared_ptr<ImageView> dst = nullptr;
		};
		using CI = CreateInfo;

		ImagePicker(CreateInfo const& ci);

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & recorder);

		void declareGUI(GUI::Context & ctx);
	};
}