#include "StatRecorder.hpp"


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
			r->declareGui(ctx, _index, _gui_show_graph);
		}

		ImGui::PopID();
	}
}