#include <vkl/App/MainWindowApp.hpp>

#include <argparse/argparse.hpp>

namespace vkl
{
	void MainWindowApp::FillArgs(argparse::ArgumentParser& args_parser)
	{
		VkApplication::FillArgs(args_parser);
		
		args_parser.add_argument("--resolution")
			.help("Set the resolution of the main window")
			.nargs(2)
			.scan<'d', unsigned int>()
		;
	}

	MainWindowApp::MainWindowApp(CreateInfo const& ci) :
		VkApplication(VkApplication::CI{
			.name = ci.name,
			.args = ci.args,
		})
	{
		
		if (ci.args.is_used("--resolution"))
		{
			auto args_resolution = ci.args.get<std::vector<unsigned int>>("--resolution");
			_desired_window_options.resolution.x = args_resolution[0];
			_desired_window_options.resolution.y = args_resolution[1];
		}

	}

	void MainWindowApp::createMainWindow()
	{
		_desired_window_options.app = this;
		_main_window = std::make_shared<VkWindow>(_desired_window_options);
	}

	VkApplication::DesiredQueuesInfo MainWindowApp::getDesiredQueuesInfo()
	{
		DesiredQueuesInfo res = VkApplication::getDesiredQueuesInfo();
		res.need_presentation = true;
		return res;
	}
}