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
		for (auto& r : _records)
		{
			delete r;
			r = nullptr;
		}
		_records.clear();
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

	void StatRecords::declareGui(GuiContext& ctx)
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
		
		
		StatRecord<TimeCountClock::rep>* frame_time_record = createRecord<TimeCountClock::rep>({
			.name = "Frame Time (CPU)",
			.scale = stat_ms_scale,
			.provider = Dyn<size_t>(&fpc.frame_time),
			.unit = "ms",
		});
		{
			StatRecord<TimeCountClock::rep>* update_time_record = frame_time_record->createChildRecord<TimeCountClock::rep>({
				.name = "Update Time (CPU)",
				.scale = stat_ms_scale,
				.provider = Dyn<size_t>(&fpc.update_time),
				.unit = "ms",
			});
			{
				StatRecord<TimeCountClock::rep>* prepare_scene_time_record = update_time_record->createChildRecord<TimeCountClock::rep>({
					.name = "Prepare Scene Time",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.prepare_scene_time),
					.unit = "ms",
				});
				StatRecord<TimeCountClock::rep>* update_scene_time_record = update_time_record->createChildRecord<TimeCountClock::rep>({
					.name = "Update Scene Time",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.update_scene_time),
					.unit = "ms",
				});
				StatRecord<TimeCountClock::rep>* descriptor_updates = update_time_record->createChildRecord<TimeCountClock::rep>({
					.name = "Descriptor Updates",
					.provider = Dyn<size_t>(&fpc.descriptor_updates),
				});
			}

			StatRecord<TimeCountClock::rep>* render_time_cpu_record = frame_time_record->createChildRecord<TimeCountClock::rep>({
				.name = "Render Time (CPU)",
				.scale = stat_ms_scale,
				.provider = Dyn<size_t>(&fpc.render_time),
				.unit = "ms",
			});
			{
				StatRecord<TimeCountClock::rep>* generate_draw_list_record = render_time_cpu_record->createChildRecord<TimeCountClock::rep>({
					.name = "Generate Scene Draw List Time",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.generate_scene_draw_list_time),
					.unit = "ms",
				});
				StatRecord<TimeCountClock::rep>* render_draw_list_record = render_time_cpu_record->createChildRecord<TimeCountClock::rep>({
					.name = "Render Scene Draw List Time",
					.scale = stat_ms_scale,
					.provider = Dyn<size_t>(&fpc.render_draw_list_time),
					.unit = "ms",
				});
				StatRecord<size_t>* draw_calls = render_time_cpu_record->createChildRecord<size_t>({
					.name = "Draw calls",
					.provider = Dyn<size_t>(&fpc.draw_calls),
				});
				StatRecord<size_t>* dispatch_calls = render_time_cpu_record->createChildRecord<size_t>({
					.name = "Dispatch calls",
					.provider = Dyn<size_t>(&fpc.dispatch_calls),
				});
				StatRecord<size_t>* pipeline_barriers = render_time_cpu_record->createChildRecord<size_t>({
					.name = "Pipeline Barriers",
					.provider = Dyn<size_t>(&fpc.pipeline_barriers),
				});
				{
					StatRecord<size_t>* buffer_barriers = pipeline_barriers->createChildRecord<size_t>({
						.name = "Buffer Barriers",
						.provider = Dyn<size_t>(&fpc.buffer_barriers),
					});
					StatRecord<size_t>* image_barriers = pipeline_barriers->createChildRecord<size_t>({
						.name = "Image Barriers",
						.provider = Dyn<size_t>(&fpc.image_barriers),
					});
					StatRecord<size_t>* layout_transitions = pipeline_barriers->createChildRecord<size_t>({
						.name = "Layout Transitions",
						.provider = Dyn<size_t>(&fpc.layout_transitions),
					});
				}
			}
		}
	}
}