#pragma once

#include "VkApplication.hpp"
#include <vkl/VkObjects/VkWindow.hpp>

namespace vkl
{
	class MainWindowApp : public VkApplication
	{
	public:

		static void FillArgs(argparse::ArgumentParser & args_parser);

	protected:

		std::shared_ptr<VkWindow> _main_window = nullptr;

		VkWindow::CreateInfo _desired_window_options = {
			.resolution = {1600, 900},
			.resizeable = true,
		};

		void createMainWindow();

		virtual DesiredQueuesInfo getDesiredQueuesInfo() override;

	public:

		struct CreateInfo
		{
			std::string name = {};
			argparse::ArgumentParser & args;
		};
		using CI = CreateInfo;

		MainWindowApp(CreateInfo const& ci);
	};
}