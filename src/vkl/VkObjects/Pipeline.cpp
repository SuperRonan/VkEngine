#include <vkl/VkObjects/Pipeline.hpp>
#include <vkl/VkObjects/VulkanExtensionsSet.hpp>

namespace vkl
{

	PipelineInstance::PipelineInstance(CreateInfo const& ci):
		AbstractInstance(ci.app, ci.name),
		_binding(ci.binding),
		_program(ci.program)
	{

	}

	PipelineInstance::~PipelineInstance()
	{
		if (_handle != VK_NULL_HANDLE)
		{
			callDestructionCallbacks();
			vkDestroyPipeline(_app->device(), _handle, nullptr);
			_handle = VK_NULL_HANDLE;
		}
	}

	void PipelineInstance::setVkNameIFP()
	{
		application()->nameVkObjectIFP(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(_handle), name());
	}

	void Pipeline::launchInstanceCreationTask()
	{
		waitForInstanceCreationIFN();
		assert(_program->hasInstanceOrIsPending());
		std::vector<std::shared_ptr<AsynchTask>> dependecies;
		if (_program->creationTask())
		{
			dependecies.push_back(_program->creationTask());
		}

		_create_instance_task = std::make_shared<AsynchTask>(AsynchTask::CI{
			.name = "Create Pipeline " + name(),
			.priority = TaskPriority::ASAP(),
			.lambda = [this]() {
				createInstanceIFP();
				return AsynchTask::ReturnType{
					.success = true,
				};
			},
			.dependencies = dependecies,
		});

		application()->threadPool().pushTask(_create_instance_task);
	}

	Pipeline::Pipeline(CreateInfo const& ci):
		ParentType(ci.app, ci.name, ci.hold_instance),
		_binding(ci.binding),
		_program(ci.program)
	{
		Callback cb{
			.callback = [this]() {
				destroyInstanceIFN();
			},
			.id = this,
		};
		_program->setInvalidationCallback(cb);
	}

	void Pipeline::destroyInstanceIFN()
	{
		waitForInstanceCreationIFN();
		ParentType::destroyInstanceIFN();
	}


	Pipeline::~Pipeline()
	{
		if (_create_instance_task)
		{
			_create_instance_task->cancel(false);
		}
		destroyInstanceIFN();
		_program->removeInvalidationCallback(this);
	}

	bool Pipeline::updateResources(UpdateContext & ctx)
	{
		if (_latest_update_tick >= ctx.updateTick())
		{
			return _latest_update_result;
		}
		_latest_update_tick = ctx.updateTick();
		bool & res = _latest_update_result = false;
		bool can_create = true;

		res |= _program->updateResources(ctx);
		can_create &= _program->hasInstanceOrIsPending();

		if (checkHoldInstance())
		{
			if (checkInstanceParamsReturnInvalid())
			{
				res = true;
			}

			if (res)
			{
				destroyInstanceIFN();
			}

			if (!_inst && can_create)
			{
				launchInstanceCreationTask();
				res = true;
			}
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