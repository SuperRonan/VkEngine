#pragma once

#include <Core/VkObjects/AbstractInstance.hpp>

#include <Core/Execution/UpdateContext.hpp>

namespace vkl
{
	class QueryPoolInstance : public AbstractInstance
	{
	protected:

		VkQueryPoolCreateFlags _flags = 0;
		VkQueryType _type = VK_QUERY_TYPE_MAX_ENUM;
		uint32_t _count = 0;
		VkQueryPipelineStatisticFlags _pipeline_statistics = 0;

		VkQueryPool _handle = VK_NULL_HANDLE;

		
		void create();

		void destroy();

		void setVkName();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			VkQueryPoolCreateFlags flags = 0;
			VkQueryType type = VK_QUERY_TYPE_MAX_ENUM;
			uint32_t count = 0;
			VkQueryPipelineStatisticFlags pipeline_statistics = 0;
		};
		using CI = CreateInfo;

		QueryPoolInstance(CreateInfo const& ci);

		virtual ~QueryPoolInstance() override;

		constexpr VkQueryPool handle()const
		{
			return _handle;
		}

		constexpr operator VkQueryPool()const
		{
			return handle();
		}

		constexpr uint32_t count() const
		{
			return _count;
		}
	};

	class QueryPool : public InstanceHolder<QueryPoolInstance>
	{
	protected:

		using ParentType = InstanceHolder<QueryPoolInstance>;
		
		VkQueryPoolCreateFlags _flags = 0;
		VkQueryType _type = VK_QUERY_TYPE_MAX_ENUM;
		Dyn<uint32_t> _count = {};
		VkQueryPipelineStatisticFlags _pipeline_statistics = 0;

		size_t _latest_update_tick = 0;


	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			VkQueryPoolCreateFlags flags = 0;
			VkQueryType type = VK_QUERY_TYPE_MAX_ENUM;
			Dyn<uint32_t> count = {};
			VkQueryPipelineStatisticFlags pipeline_statistics = 0;
			Dyn<bool> hold_instance = {};
		};
		using CI = CreateInfo;

		QueryPool(CreateInfo const& ci);

		virtual ~QueryPool() override = default;

		const Dyn<uint32_t>& count()const
		{
			return _count;
		}

		Dyn<uint32_t>& count()
		{
			return _count;
		}
		
		void createInstance();

		void updateResources(UpdateContext & ctx);
	};
}