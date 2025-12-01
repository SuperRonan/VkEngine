#pragma once

#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	class DemoPanel final : public Panel
	{
	protected:

	public:

		DemoPanel(VkApplication * app = nullptr);

		virtual ~DemoPanel() final override;

		virtual void declare(Context& ctx) final override;
	};
}