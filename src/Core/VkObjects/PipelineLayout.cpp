#include "PipelineLayout.hpp"

namespace vkl
{
	PipelineLayoutInstance::PipelineLayoutInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_sets(ci.sets),
		_push_constants(ci.push_constants)
	{
		
		MyVector<VkDescriptorSetLayout> sets_layout;
		sets_layout.resize(_sets.size());
		for(size_t i=0; i<_sets.size(); ++i)
		{
			assert(_sets[i]);
			sets_layout[i] = _sets[i]->handle();
		}
		VkPipelineLayoutCreateInfo vk_ci {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = _flags,
			.setLayoutCount = sets_layout.size32(),
			.pSetLayouts = sets_layout.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(_push_constants.size()),
			.pPushConstantRanges = _push_constants.data(),
		};
		create(vk_ci);
		setVkName();
	}

	PipelineLayoutInstance::~PipelineLayoutInstance()
	{
		if (_layout != VK_NULL_HANDLE)
		{
			destroy();
		}
	}

	void PipelineLayoutInstance::create(VkPipelineLayoutCreateInfo const& ci)
	{
		VK_CHECK(vkCreatePipelineLayout(_app->device(), &ci, nullptr, &_layout), "Failed to create a pipeline layout.");
	}

	void PipelineLayoutInstance::setVkName()
	{
		application()->nameVkObjectIFP(VkDebugUtilsObjectNameInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT,
			.objectHandle = reinterpret_cast<uint64_t>(_layout),
			.pObjectName = name().c_str(),
		});
	}

	void PipelineLayoutInstance::destroy()
	{
		vkDestroyPipelineLayout(_app->device(), _layout, nullptr);
		_layout = VK_NULL_HANDLE;
	}







	PipelineLayout::PipelineLayout(CreateInfo const& ci) :
		ParentType(ci.app, ci.name, ci.hold_instance),
		_sets(ci.sets),
		_push_constants(ci.push_constants)
	{
		_is_dynamic = false;
		for (size_t i = 0; i < _sets.size(); ++i)
		{
			if (_sets[i])
			{
				if (_sets[i]->isDynamic())
				{
					_sets[i]->setInvalidationCallback(Callback{
						.callback = [this]()
						{
							destroyInstanceIFN();
						},
						.id = this,
					});
					_is_dynamic = true;
				}
				else
				{

				}
			}
		}
	}

	PipelineLayout::~PipelineLayout()
	{
		for (size_t i = 0; i < _sets.size(); ++i)
		{
			if (_sets[i] && _sets[i]->isDynamic())
			{
				_sets[i]->removeInvalidationCallback(this);
			}
		}
	}

	void PipelineLayout::createInstance()
	{
		std::vector<std::shared_ptr<DescriptorSetLayoutInstance>> sets(_sets.size());
		for (size_t i = 0; i < _sets.size(); ++i)
		{
			if(_sets[i])
				sets[i] = _sets[i]->instance();
		}
		_inst = std::make_shared<PipelineLayoutInstance>(PipelineLayoutInstance::CI{
			.app = application(),
			.name = name(),
			.sets = std::move(sets),
			.push_constants = _push_constants,
		});
	}

	bool PipelineLayout::updateResources(UpdateContext& ctx)
	{
		bool res = false;
		if (isDynamic())
		{
			if (ctx.updateTick() > _update_tick)
			{
				_update_tick = ctx.updateTick();
				if (checkHoldInstance())
				{
					for (size_t i = 0; i < _sets.size(); ++i)
					{
						if (_sets[i])
						{
							res |= _sets[i]->updateResources(ctx);
						}
					}

					if (!_inst)
					{
						createInstance();
						res = true;
					}
				}
			}
		}

		assert(!!_inst);

		return res;
	}
}