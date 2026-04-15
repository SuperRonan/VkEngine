#include <vkl/GUI/DescriptorInstancePanel.hpp>

namespace vkl::GUI
{
	void AbstractDescriptorPanel::declareInstance(Context& ctx)
	{
		if (_target)
		{
			bool res = _instance_panel.declareInline(ctx, target()->instancePtr());
			if (res)
			{
				target()->destroyInstanceIFN();
			}
		}
		else
		{
			_instance_panel.clear();
		}
	}
}