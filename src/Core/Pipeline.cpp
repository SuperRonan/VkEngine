#include "Pipeline.hpp"

namespace vkl
{
	PipelineInstance::~PipelineInstance()
	{
		if (_handle != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(_app->device(), _handle, nullptr);
			_handle = VK_NULL_HANDLE;
		}
	}

	PipelineInstance::PipelineInstance(GraphicsCreateInfo const& gci) :
		VkObject(gci.app, gci.name),
		_binding(VK_PIPELINE_BIND_POINT_GRAPHICS),
		_program(gci.program),
		_render_pass(gci.render_pass)
	{

		VK_CHECK(vkCreateGraphicsPipelines(_app->device(), nullptr, 1, &gci.assemble(), nullptr, &_handle), "Failed to create a graphics pipeline.");
	}

	PipelineInstance::PipelineInstance(ComputeCreateInfo const& cci) :
		VkObject(cci.app, cci.name),
		_binding(VK_PIPELINE_BIND_POINT_COMPUTE),
		_program(cci.program)
	{
		const VkComputePipelineCreateInfo ci{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = cci.program->shader()->getPipelineShaderStageCreateInfo(),
				.layout = *_program->pipelineLayout(),
		};
		VK_CHECK(vkCreateComputePipelines(_app->device(), nullptr, 1, &ci, nullptr, &_handle), "Failed to create a compute pipeline.");
	}

	void Pipeline::createInstance()
	{
		if (_binding == VK_PIPELINE_BIND_POINT_GRAPHICS)
		{
			PipelineInstance::GraphicsCreateInfo gci;
			gci.app = application();
			gci.name = name();
			gci.vertex_input = _gci.vertex_input;
			gci.input_assembly = _gci.input_assembly;
			gci.viewports = _gci.viewports;
			gci.scissors = _gci.scissors;
			gci.rasterization = _gci.rasterization;
			gci.multisampling = _gci.multisampling;
			gci.depth_stencil = _gci.depth_stencil;
			gci.attachements_blends = _gci.attachements_blends;
			gci.render_pass = _gci.render_pass;
			gci.dynamic = _gci.dynamic;
			gci.program = std::dynamic_pointer_cast<GraphicsProgramInstance>(_gci.program->instance());

			_inst = std::make_shared<PipelineInstance>(gci);
		}
		else if (_binding == VK_PIPELINE_BIND_POINT_COMPUTE)
		{
			PipelineInstance::ComputeCreateInfo cci{
				.app = application(),
				.name = name(),
				.program = std::dynamic_pointer_cast<ComputeProgramInstance>(_cci.program->instance()),
			};

			_inst = std::make_shared<PipelineInstance>(cci);
		}
	}

	void Pipeline::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	Pipeline::Pipeline(GraphicsCreateInfo const& gci) :
		ParentType(gci.app, gci.name),
		_gci(gci),
		_binding(VK_PIPELINE_BIND_POINT_GRAPHICS),
		_render_pass(gci.render_pass),
		_program(gci.program)
	{
		_program->addInvalidationCallback({
			.callback = [&]() {
				destroyInstance();
			},
			.id = this,
		});
	}

	Pipeline::Pipeline(ComputeCreateInfo const& cci) :
		ParentType(cci.app, cci.name),
		_cci(cci),
		_binding(VK_PIPELINE_BIND_POINT_COMPUTE),
		_program(cci.program)
	{
		_program->addInvalidationCallback({
			.callback = [&]() {
				destroyInstance();
			},
			.id = this,
		});
	}

	Pipeline::~Pipeline()
	{
		destroyInstance();
		_program->removeInvalidationCallbacks(this);
	}

	bool Pipeline::updateResources()
	{
		bool res = false;

		res |= _program->updateResources();

		if (!_inst)
		{
			createInstance();
			res = true;
		}

		return res;
	}
}