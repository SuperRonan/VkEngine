#include "PipelineLayout.hpp"

namespace vkl
{
	PipelineLayout::PipelineLayout(VkApplication* app, VkPipelineLayoutCreateInfo const& ci) :
		VkObject(app)
	{
		create(ci);
	}

	PipelineLayout::~PipelineLayout()
	{
		if (_layout != VK_NULL_HANDLE)
		{
			destroy();
		}
	}

	void PipelineLayout::create(VkPipelineLayoutCreateInfo const& ci)
	{
		VK_CHECK(vkCreatePipelineLayout(_app->device(), &ci, nullptr, &_layout), "Failed to create a pipeline layout.");
	}

	void PipelineLayout::destroy()
	{
		vkDestroyPipelineLayout(_app->device(), _layout, nullptr);
		_layout = VK_NULL_HANDLE;
	}
}