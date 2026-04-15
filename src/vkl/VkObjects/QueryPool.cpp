#include <vkl/VkObjects/QueryPool.hpp>

namespace vkl
{
	void QueryPoolInstance::create()
	{
		assert(!_handle);
		VkQueryPoolCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = _flags,
			.queryType = _type,
			.queryCount = _count,
			.pipelineStatistics = _pipeline_statistics,
		};
		vkCreateQueryPool(device(), &ci, nullptr, &_handle);
	}

	void QueryPoolInstance::destroy()
	{
		assert(_handle);
		vkDestroyQueryPool(device(), _handle, nullptr);
		_handle = nullptr;
	}

	void QueryPoolInstance::setVkName()
	{
		application()->nameVkObjectIFP(VK_OBJECT_TYPE_QUERY_POOL, reinterpret_cast<uint64_t>(_handle), name());
	}

	QueryPoolInstance::~QueryPoolInstance()
	{
		if (_handle)
		{
			destroy();
		}
	}

	QueryPoolInstance::QueryPoolInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_flags(ci.flags),
		_type(ci.type),
		_count(ci.count),
		_pipeline_statistics(ci.pipeline_statistics)
	{
		create();
	}



	QueryPool::QueryPool(CreateInfo const& ci) :
		ParentType(ci.app, ci.name, ci.hold_instance),
		_flags(ci.flags),
		_type(ci.type),
		_count(ci.count),
		_pipeline_statistics(ci.pipeline_statistics)
	{

	}

	void QueryPool::createInstance()
	{
		_instance = std::make_shared<QueryPoolInstance>(QueryPoolInstance::CI{
			.app = application(),
			.name = name(),
			.flags = _flags,
			.type = _type,
			.count = *_count,
			.pipeline_statistics = _pipeline_statistics,
		});
	}

	void QueryPool::updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res)
	{
		if (_instance)
		{
			if (!res.invalidated)
			{
				uint32_t count = *_count;
				if (count != instance()->count())
				{
					res.invalidated = true;
				}
			}
			if (res.invalidated)
			{
				destroyInstanceIFN();
			}
		}
		if (!_instance)
		{
			res.created = true;
			createInstance();
		}
	}
}