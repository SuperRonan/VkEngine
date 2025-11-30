#include <vkl/Execution/PerformanceReport.hpp>

#include <vkl/Execution/FramePerformanceCounters.hpp>
#include <vkl/Execution/FramePerfReport.hpp>
#include <vkl/Execution/ExecutionStackReport.hpp>

namespace vkl
{
	PerformanceReport::PerformanceReport(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_stat_records(std::make_unique<StatRecords>(StatRecords::CI{
			.name = name() + ".Statistics",
			.memory = ci.memory,
			.period = ci.period,
		}))
	{
		std::memset(&_perf_counters, 0, sizeof(FramePerfCounters));
		_stat_records->createCommonRecords(_perf_counters);
	}

	void PerformanceReport::declareGUI(GUI::Context& ctx)
	{
		ImGui::PushID(this);
		_stat_records->declareGui(ctx);
		ImGui::Separator();
		_generate_frame_report = ImGui::Button("Generate Frame Report");
		if (_frame_perf_report)
		{
			const bool ready = _frame_perf_report->ready_for_display;
			if (ready)
			{
				_frame_perf_report->report->declareGUI(ctx);
			}
			else
			{
				ImGui::Text("Waiting on report to generate...");
			}
		}
		ImGui::PopID();
	}

	void PerformanceReport::advance()
	{
		// TODO in a separate thread
		_stat_records->advance();
	}
}