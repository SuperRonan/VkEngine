#pragma once

#include <Core/App/VkApplication.hpp>

#include <Core/VkObjects/QueryPool.hpp>

namespace vkl
{
	class ExecutionStackReport;

	struct FramePerfReport
	{
		std::shared_ptr<ExecutionStackReport> report;
		std::shared_ptr<QueryPool> timestamp_query_pool;
		MyVector<uint8_t> query_data;

		std::atomic<uint32_t> finish_counter = 0;
		uint32_t max_timestamp_bits = 0;

		uint32_t query_count = 0;

		bool emit_mt = false;
		std::atomic<bool> ready_for_display = false;
		bool query_reseted = false;

		std::shared_ptr<AsynchTask> _finish_task = nullptr;

		void push();

		void pop();

		void clear();

		VkResult queryResults(VkQueryResultFlags query_flags = 0);

		void finish();
	};
}