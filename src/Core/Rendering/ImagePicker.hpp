#pragma once

#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Sampler.hpp>

#include <Core/Execution/Module.hpp>
#include <Core/Execution/Executor.hpp>

#include <Core/IO/GuiContext.hpp>
#include <Core/IO/ImGuiUtils.hpp>

#include <Core/Commands/TransferCommand.hpp>
#include <Core/Commands/GraphicsTransferCommands.hpp>
#include <Core/Commands/ComputeCommand.hpp>



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

		void declareGUI(GuiContext & ctx);
	};
}