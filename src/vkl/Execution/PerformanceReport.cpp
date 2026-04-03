#include <vkl/Execution/PerformanceReport.hpp>

#include <vkl/Execution/FramePerformanceCounters.hpp>
#include <vkl/Execution/FramePerfReport.hpp>
#include <vkl/Execution/ExecutionStackReport.hpp>

#include <vkl/GUI/InlinePanel.hpp>

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

	void PerformanceReport::advance()
	{
		// TODO in a separate thread
		_stat_records->advance();
	}

	namespace GUI
	{
		class PerformanceReportInspector : public Panel
		{
		protected:
			std::shared_ptr<PerformanceReport> _target;
		public:

			PerformanceReportInspector(std::shared_ptr<PerformanceReport> const& target) :
				Panel(target->application(), std::format("{}", target->name())),
				_target(target)
			{

			}

			virtual void declareInline(GUI::Context& ctx) override
			{
				_target->_stat_records->declareGui(ctx);
				ImGui::Separator();
				_target->_generate_frame_report = ImGui::Button("Generate Frame Report");
				if (_target->_frame_perf_report)
				{
					const bool ready = _target->_frame_perf_report->ready_for_display;
					if (ready)
					{
						_target->_frame_perf_report->report->declareGUI(ctx);
					}
					else
					{
						ImGui::Text("Waiting on report to generate...");
					}
				}
			}
		};
	}

	std::shared_ptr<GUI::Panel> PerformanceReport::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return std::make_shared<GUI::PerformanceReportInspector>(std::static_pointer_cast<PerformanceReport>(shared_this));
	}
}