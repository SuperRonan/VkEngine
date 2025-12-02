#pragma once

#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	// Singleton
	class DemoPanel final : public Panel
	{
	protected:

		static std::shared_ptr<DemoPanel> _singleton;

		DemoPanel(VkApplication * app = nullptr);

	public:
		
		static std::shared_ptr<DemoPanel> GetSingleton();

		virtual ~DemoPanel() final override;

		virtual void declare(Context& ctx) final override;
	};
}