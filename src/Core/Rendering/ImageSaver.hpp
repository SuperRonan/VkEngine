#pragma once

#include <Core/Execution/Module.hpp>
#include <Core/Execution/Executor.hpp>

#include <Core/IO/GuiContext.hpp>

#include <Core/VkObjects/ImageView.hpp>

#include <Core/IO/ImGuiUtils.hpp>

namespace vkl
{
	class ImageSaver : public Module
	{
	protected:

		std::shared_ptr<ImageView> _src = nullptr;
		std::filesystem::path _dst_folder = {};
		std::string _dst_folder_str = {};
		std::string _dst_filename = {};
		std::string _extension;
		size_t _index = 0;

		bool _save_image = false;
		bool _save_in_separate_thread = true;

		uint32_t _pending_capacity = 2;
		
		std::mutex _mutex;
		std::deque<std::shared_ptr<AsynchTask>> _pending_tasks;

		ImGuiListSelection _gui_extension;
		int _jpg_quality = 100;
		bool _create_folder_ifn = true;

		void setExtension();

	public:

		struct CreateInfo
		{
			VkApplication * app;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			std::filesystem::path dst_folder = {};
			std::string dst_filename = {};
		};
		using CI = CreateInfo;

		ImageSaver(CreateInfo const& ci);

		virtual ~ImageSaver() override;

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & exec);

		void declareGUI(GuiContext & ctx);
	};
}