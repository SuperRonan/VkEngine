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
			.verbosity = AsynchTask::Verbosity::High,
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
			_create_instance_task->cancel();
		}
		destroyInstanceIFN();
		_program->removeInvalidationCallback(this);
	}

	void Pipeline::updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res)
	{
		bool can_create = true;
		res.invalidated |= _program->updateResources(ctx).invalidated;
		can_create &= _program->hasInstanceOrIsPending();

		if (_inst)
		{
			if (checkInstanceParamsReturnInvalid())
			{
				res.invalidated = true;
			}

			if (res.invalidated)
			{
				destroyInstanceIFN();
			}
		}

		if (!_inst && can_create)
		{
			res.created = true;
			launchInstanceCreationTask();
		}
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