#pragma once

#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	class TestPanel : public Panel
	{
	protected:

		void* _internal;

	public:

		TestPanel(VkApplication* app);

		virtual ~TestPanel() override;

		virtual void declareInline(Context& ctx);

		void* getInternal() const
		{
			return _internal;
		}
	};
}