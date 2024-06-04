#include <Core/Execution/FramePerfReport.hpp>
#include <Core/Execution/ExecutionStackReport.hpp>


namespace vkl
{
	void FramePerfReport::push()
	{
		++finish_counter;
	}

	void FramePerfReport::pop()
	{
		assert(finish_counter != 0);
		// fetch before sub
		const uint32_t a = finish_counter.fetch_sub(1);
		if (a <= 1)
		{
			finish();
		}
	}

	void FramePerfReport::clear()
	{
		report->clear();
		query_data.clear();
		finish_counter = 0;
		max_timestamp_bits = 0;
		query_count = 0;
		ready_for_display = false;
		query_reseted = false;
		if (emit_mt && _finish_task)
		{
			_finish_task->reset();
		}
	}

	template <std::unsigned_integral UInt, bool EXTRA>
	struct TimestampQueryResult;

	template <std::unsigned_integral UInt>
	struct TimestampQueryResult<UInt, false>
	{
		UInt result;
	};

	template <std::unsigned_integral UInt>
	struct TimestampQueryResult<UInt, true>
	{
		UInt result;
		UInt available_or_status;
	};

	VkResult FramePerfReport::queryResults(VkQueryResultFlags query_flags)
	{
		VkResult result;
		if (max_timestamp_bits > 0 && query_count > 0)
		{
			const uint32_t timestamp_bits = (max_timestamp_bits <= 32) ? 32 : 64;
			const uint64_t timestamp_bytes = timestamp_bits / 8;
			bool pair = false;

			if (timestamp_bits == 64)
			{
				query_flags |= VK_QUERY_RESULT_64_BIT;
			}

			uint32_t query_result_size = timestamp_bytes;

			// TODO status
			if (query_flags & (VK_QUERY_RESULT_WITH_AVAILABILITY_BIT))
			{
				query_result_size *= 2;
				pair = true;
			}

			QueryPoolInstance* queries = timestamp_query_pool->instance().get();
			assert(queries);
			query_data.resize(query_result_size * query_count);
			void* p_data = query_data.data();
			result = vkGetQueryPoolResults(queries->device(), *queries, 0, query_count, query_data.byte_size(), p_data, query_result_size, query_flags);

			if (result == VK_SUCCESS)
			{
				const uint32_t qc = query_count;
				ExecutionStackReport::GetQueryResultFn get_query_result_f;
				auto get_query_result_lambda = [p_data, qc] <std::unsigned_integral UInt, bool PAIR> () -> ExecutionStackReport::GetQueryResultFn
				{
					return [p_data, qc](uint32_t index, uint64_t& result, uint32_t* available)
						{
							using TQR = TimestampQueryResult<UInt, PAIR>;

							if (index < qc)
							{
								const TQR* tqr = reinterpret_cast<const TQR*>(p_data) + index;
								result = uint64_t(tqr->result);
								if (available)
								{
									if constexpr (PAIR)
									{
										*available = static_cast<uint32_t>(tqr->available_or_status);
									}
									else
									{
										*available = 1;
									}
								}
							}
							else
							{
								result = 0;
								if (available)	*available = 0;
							}
						};
				};
				if (timestamp_bits == 32)
				{
					if (pair)
					{
						get_query_result_f = get_query_result_lambda.template operator() < uint32_t, true > ();
					}
					else
					{
						get_query_result_f = get_query_result_lambda.template operator() < uint32_t, false > ();
					}

				}
				else if (timestamp_bits == 64)
				{
					if (pair)
					{
						get_query_result_f = get_query_result_lambda.template operator() < uint64_t, true > ();
					}
					else
					{
						get_query_result_f = get_query_result_lambda.template operator() < uint64_t, false > ();
					}
				}
				report->aquireQueryResults(get_query_result_f);
			}
			else
			{
				NOT_YET_IMPLEMENTED;
			}
		}
		else
		{
			result = VK_INCOMPLETE;
		}
		return result;
	}

	void FramePerfReport::finish()
	{
		auto f = [this](bool mt)
		{
			VkQueryResultFlags query_flags = 0;
			if (mt)
			{
				query_flags |= VK_QUERY_RESULT_WAIT_BIT;
			}
			queryResults(query_flags);
			report->prepareForGUI();
			ready_for_display = true;
		};

		if (emit_mt)
		{
			if (!_finish_task)
			{
				_finish_task = std::make_shared<AsynchTask>(AsynchTask::CI{
					.name = report->name(),
					.priority = TaskPriority::Soon(),
					.lambda = [f]()
					{
						f(true);
						return AsynchTask::ReturnType{
							.success = true,
						};
					},
				});
			}
			report->application()->threadPool().pushTask(_finish_task);
		}
		else
		{
			f(false);
		}
	}
}