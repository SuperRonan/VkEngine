#include "Pipeline.hpp"

namespace vkl
{
	PipelineInstance::~PipelineInstance()
	{
		if (_handle != VK_NULL_HANDLE)
		{
			callDestructionCallbacks();
			vkDestroyPipeline(_app->device(), _handle, nullptr);
			_handle = VK_NULL_HANDLE;
		}
	}

	PipelineInstance::PipelineInstance(GraphicsCreateInfo const& gci) :
		AbstractInstance(gci.app, gci.name),
		_binding(VK_PIPELINE_BIND_POINT_GRAPHICS),
		_program(gci.program),
		_render_pass(gci.render_pass)
	{

		VK_CHECK(vkCreateGraphicsPipelines(_app->device(), nullptr, 1, &gci.assemble(), nullptr, &_handle), "Failed to create a graphics pipeline.");
	}

	PipelineInstance::PipelineInstance(ComputeCreateInfo const& cci) :
		AbstractInstance(cci.app, cci.name),
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
		waitForInstanceCreationIFN();

		std::vector<std::shared_ptr<AsynchTask>> dependecies;
		if (_program->creationTask())
		{
			dependecies.push_back(_program->creationTask());
		}

		_create_instance_task = std::make_shared<AsynchTask>(AsynchTask::CI{
			.name = "Create Pipeline " + name(),
			.priority = TaskPriority::ASAP(),
			.lambda = [this]() {
		
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
					gci.line_raster = _gci.line_raster;
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
				else if (_binding == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR)
				{
					
				}

				return AsynchTask::ReturnType{
					.success = true,
				};
			},
			.dependencies = dependecies,
		});

		application()->threadPool().pushTask(_create_instance_task);
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
		waitForInstanceCreationIFN();
		destroyInstance();
		_program->removeInvalidationCallbacks(this);
	}

	bool Pipeline::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		res |= _program->updateResources(ctx);

		if (!_inst)
		{
			createInstance();
			res = true;
		}

		return res;
	}

	void Pipeline::waitForInstanceCreationIFN()
	{
		if (_create_instance_task)
		{
			_create_instance_task->waitIFN();
			assert(_create_instance_task->isSuccess());
			_create_instance_task = nullptr;
		}
	}

	std::shared_ptr<PipelineInstance> Pipeline::getInstanceWaitIFN()
	{
		waitForInstanceCreationIFN();
		return instance();
	}
}