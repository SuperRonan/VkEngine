#include <vkl/Utils/StatRecorder.hpp>
#include <vkl/Utils/TickTock.hpp>


namespace vkl
{
	StatRecords::StatRecords(CreateInfo const& ci):
		_name(ci.name),
		_memory(ci.memory),
		_period(ci.period),
		_latest_time_point(Clock::now())
	{
		
	}

	StatRecords::~StatRecords()
	{

	}

	void StatRecords::advance()
	{
		++_index;
		if (_index >= _memory)
		{
			_index = 0;	
		}

		++_iter_counter_since_avg;
		Clock::time_point now = Clock::now();
		size_t avg_begin = 0;
		size_t avg_len = 0;
		if ((now - _latest_time_point) > _period)
		{
			_latest_time_point = now;
			if (_iter_counter_since_avg >= _memory)
			{
				avg_begin = 0;
				avg_len = _memory;
			}
			else
			{
				if(_index >= _iter_counter_since_avg)
					avg_begin = _index - _iter_counter_since_avg;
				else
				{
					avg_begin = (_index + _memory) - _iter_counter_since_avg;
				}
					
				avg_len = _iter_counter_since_avg;
			}
			_iter_counter_since_avg = 0;
		}


		for (auto& r : _records)
		{
			r->advance(_index);
			if (avg_len > 0)
			{
				r->computeAverage(avg_begin, avg_len);
			}
		}
	}

	void StatRecords::declareGui(GUI::Context& ctx)
	{
		ImGui::PushID(name().c_str());

		ImGui::Text(name().c_str());

		//ImGui::Checkbox("Show graphs", &_gui_show_graph);

		for (auto& r : _records)
		{
			r->declareGui(ctx, _index, Pack64{.floating = -1}, _gui_show_graph);
		}

		ImGui::PopID();
	}



	void StatRecords::createCommonRecords(FramePerfCounters& fpc)
	{
		using TimeCountClock = std::TickTock_hrc::Clock_t;
		
		const double stat_ms_scale = []()
		{
			using p = TimeCountClock::period;
			using r = std::ratio_divide<p, std::milli>;
			return double(r::num) / double(r::den);
		}();
		
		
		auto frame_time_record = createRecord<TimeCountClock::rep>({
			.name = "Frame Time (CPU)",
			.scale = stat_ms_scale,
			.provider = Dyn<size_t>(&fpc.frame_time),
			.unit = "ms",
		});
		{
			auto update_time_record = frame_time_record->createChildRecord<TimeCountClock::rep>({
				.name = "Update Time (CPU)",
				.scale = stat_ms_scale,
				.provider = Dyn<size_t>(&fpc.update_time),
				.unit = "ms",
			});
			{
				auto prepare_scene_time_record = update_time_record->createChildRecord<TimeCountClock::rep>({
					.name = "Prepare Scene Time",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.prepare_scene_time),
					.unit = "ms",
					});
				auto update_scene_time_record = update_time_record->createChildRecord<TimeCountClock::rep>({
					.name = "Update Scene Time",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.update_scene_time),
					.unit = "ms",
					});
				auto update_main_modules = update_time_record->createChildRecord<TimeCountClock::rep>({
					.name = "Update executor",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.exec_update_time),
					.unit = "ms",
					});
				auto exec_update = update_time_record->createChildRecord<TimeCountClock::rep>({
					.name = "Update main Modules",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.main_script_modules_time),
					.unit = "ms",
					});
				auto descriptor_updates = update_time_record->createChildRecord<TimeCountClock::rep>({
					.name = "Descriptor Updates",
					.provider = Dyn<size_t>(&fpc.descriptor_updates),
				});
			}

			auto render_time_cpu_record = frame_time_record->createChildRecord<TimeCountClock::rep>({
				.name = "Render Time (CPU)",
				.scale = stat_ms_scale,
				.provider = Dyn<size_t>(&fpc.render_time),
				.unit = "ms",
			});
			{
				auto generate_draw_list_record = render_time_cpu_record->createChildRecord<TimeCountClock::rep>({
					.name = "Generate Scene Draw List Time",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.generate_scene_draw_list_time),
					.unit = "ms",
				});
				auto render_draw_list_record = render_time_cpu_record->createChildRecord<TimeCountClock::rep>({
					.name = "Render Scene Draw List Time",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.render_draw_list_time),
					.unit = "ms",
				});
				auto draw_calls = render_time_cpu_record->createChildRecord<size_t>({
					.name = "Draw calls",
					.provider = Dyn<size_t>(&fpc.draw_calls),
					.ignore_parent_avg = true,
					});
				auto dispatch_calls = render_time_cpu_record->createChildRecord<size_t>({
					.name = "Dispatch calls",
					.provider = Dyn<size_t>(&fpc.dispatch_calls),
					.ignore_parent_avg = true,
					});
				auto pipeline_barriers = render_time_cpu_record->createChildRecord<size_t>({
					.name = "Pipeline Barriers",
					.provider = Dyn<size_t>(&fpc.pipeline_barriers),
					.ignore_parent_avg = true,
				});
				{
					auto buffer_barriers = pipeline_barriers->createChildRecord<size_t>({
						.name = "Buffer Barriers",
						.provider = Dyn<size_t>(&fpc.buffer_barriers),
						.ignore_parent_avg = true,
					});
					auto image_barriers = pipeline_barriers->createChildRecord<size_t>({
						.name = "Image Barriers",
						.provider = Dyn<size_t>(&fpc.image_barriers),
						.ignore_parent_avg = true,
					});
					auto layout_transitions = pipeline_barriers->createChildRecord<size_t>({
						.name = "Layout Transitions",
						.provider = Dyn<size_t>(&fpc.layout_transitions),
						.ignore_parent_avg = true,
					});
				}
			}
		}
	}
}