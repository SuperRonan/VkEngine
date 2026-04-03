#pragma once

#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Sampler.hpp>

#include <vkl/Execution/Module.hpp>
#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/TransferCommand.hpp>
#include <vkl/Commands/GraphicsTransferCommands.hpp>
#include <vkl/Commands/ComputeCommand.hpp>



namespace vkl
{
	namespace GUI
	{
		class ImagePickerInspector;
	}

	class ImagePicker : public Module
	{
	protected:

		MyVector<std::shared_ptr<ImageView>> _sources = {};

		std::shared_ptr<ImageView> _dst = nullptr;

		std::shared_ptr<BlitImage> _blitter = nullptr;

		uint32_t _source_index = 0;
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

		std::shared_ptr<ImageView> source() const
		{
			return _source_index < _sources.size32() ? _sources[_source_index] : nullptr;
		}

		using InspectorType = GUI::ImagePickerInspector;
		friend class InspectorType;
		virtual std::shared_ptr<GUI::Panel> makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx) override;
	};
}