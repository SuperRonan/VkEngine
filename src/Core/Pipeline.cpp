#include "Pipeline.hpp"

namespace vkl
{
	Pipeline::~Pipeline()
	{
		if (_handle != VK_NULL_HANDLE)
		{
			destroyPipeline();
		}
	}

	Pipeline::Pipeline(VkApplication* app, VkGraphicsPipelineCreateInfo const& ci) :
		VkObject(app)
	{
		createPipeline(ci);
	}

	Pipeline::Pipeline(VkApplication* app, VkComputePipelineCreateInfo const& ci) :
		VkObject(app)
	{
		createPipeline(ci);
	}
	
	void Pipeline::destroyPipeline()
	{
		vkDestroyPipeline(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	void Pipeline::createPipeline(VkGraphicsPipelineCreateInfo const& gpci)
	{
		VK_CHECK(vkCreateGraphicsPipelines(_app->device(), nullptr, 1, &gpci, nullptr, &_handle), "Failed to create a graphics pipeline.");
	}

	void Pipeline::createPipeline(VkComputePipelineCreateInfo const& cpci)
	{
		VK_CHECK(vkCreateComputePipelines(_app->device(), nullptr, 1, &cpci, nullptr, &_handle), "Failed to create a compute pipeline.");
	}
}