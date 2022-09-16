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

	Pipeline::Pipeline(VkApplication* app, GraphicsCreateInfo & ci) :
		VkObject(app),
		_program(ci._program)
	{
		createPipeline(ci._pipeline_ci);
	}

	Pipeline::Pipeline(VkApplication* app, std::shared_ptr<ComputeProgram> compute_program) :
		VkObject(app),
		_binding(VK_PIPELINE_BIND_POINT_COMPUTE),
		_program(compute_program)
	{
		VkComputePipelineCreateInfo ci{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = compute_program->shader()->getPipelineShaderStageCreateInfo(),
				.layout = compute_program->pipelineLayout(),
		};
		createPipeline(ci);
	}

	Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
	{
		VkObject::operator=(std::move(other));
		std::swap(_handle, other._handle);
		std::swap(_binding, other._binding);
		std::swap(_program, other._program);
		return *this;
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