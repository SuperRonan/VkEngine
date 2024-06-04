#pragma once

#include <Core/Execution/Module.hpp>
#include <Core/Execution/FramePerformanceCounters.hpp>
#include <Core/Utils/StatRecorder.hpp>

#include <Core/IO/GuiContext.hpp>

namespace vkl
{
	struct FramePerfReport;
	

	class PerformanceReport : public Module
	{
	public:
		using Clock = StatRecords::Clock;
	protected:

		Clock::duration _period = 1s;

		FramePerfCounters _perf_counters = {};
		std::unique_ptr<StatRecords> _stat_records = nullptr;
		std::shared_ptr<FramePerfReport> _frame_perf_report = nullptr;

		bool _generate_frame_report = false;

	public:
		
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Clock::duration period = 1s;
			size_t memory = 256;
		};
		using CI = CreateInfo;

		PerformanceReport(CreateInfo const& ci);

		virtual ~PerformanceReport() override = default;

		void declareGUI(GuiContext & ctx);

		std::unique_ptr<StatRecords> const& statRecords()const
		{
			return _stat_records;
		}

		FramePerfCounters const& framePerfCounter()const
		{
			return _perf_counters;
		}

		FramePerfCounters & framePerfCounter()
		{
			return _perf_counters;
		}

		bool generateFrameReport()const
		{
			return _generate_frame_report;
		}

		void setFramePerfReport(std::shared_ptr<FramePerfReport> const& r)
		{
			_frame_perf_report = r;
		}

		void advance();
	};
}