#include <Core/Execution/ExecutionStackReport.hpp>

namespace vkl
{
	ExecutionStackReport::ExecutionStackReport(CreateInfo const& ci):
		Module(ci.app, ci.name)
	{}

	void ExecutionStackReport::clear()
	{
		_strings.clear();
		_stack.clear();
		_stack_top = Index(-1);
		_begin_timepoint = {};
	}

	ExecutionStackReport::Segment& ExecutionStackReport::push(std::string_view label, Color const& color)
	{
		const Index depth = (_stack_top != Index(-1)) ? (_stack[_stack_top].depth + 1) : 0;
		const Range label_range = Range{
			.begin = static_cast<Range::Index>(_strings.pushBack(label)), 
			.len = static_cast<Range::Index>(label.size()),
		}; 
		_stack.push_back(Segment{
			.depth = depth,
			.parent = _stack_top,
			.label_range = label_range,
			.color = color,
		});
		Segment & res = _stack.back();
		_stack_top = static_cast<Index>(_stack.size()) - 1;
		return res;
	}

	ExecutionStackReport::Segment* ExecutionStackReport::getStackTop()
	{
		Segment* res = nullptr;
		if (_stack_top != Index(-1))
		{
			res = _stack.data() + _stack_top;
		}
		return res;
	}

	void ExecutionStackReport::pop()
	{
		assert(_stack_top != Index(-1));
		_stack_top = _stack[_stack_top].parent;
	}

	void ExecutionStackReport::aquireQueryResults(GetQueryResultFn const& get_query_result_f)
	{
		for (size_t i = 0; i < _stack.size(); ++i)
		{
			Segment & seg = _stack[i];
			if (seg.begin_timestamp != uint64_t(-1))
			{
				// 0 == not available
				// 0 != available 
				uint32_t available = 0;
				get_query_result_f(static_cast<uint32_t>(seg.begin_timestamp), seg.begin_timestamp, &available);
				if (available == 0)
				{
					seg.begin_timestamp = uint64_t(-1);
				}
				else
				{
					// -1 is reserved for invalid value
					seg.begin_timestamp = std::min(seg.begin_timestamp, std::numeric_limits<uint64_t>::max() - 1);
				}
				if (seg.end_timestamp != uint64_t(-1))
				{
					available = 0;
					get_query_result_f(static_cast<uint32_t>(seg.end_timestamp), seg.end_timestamp, &available);
					if (available == 0)
					{
						seg.end_timestamp = uint64_t(-1);
					}
					else
					{
						// -1 is reserved for invalid value
						seg.end_timestamp = std::min(seg.end_timestamp, std::numeric_limits<uint64_t>::max() - 1);
					}
				}
			}
		}
	}

	void ExecutionStackReport::prepareForGUI()
	{
		// ns per tick
		const double device_period = application()->deviceProperties().props2.properties.limits.timestampPeriod;
		constexpr const std::array units = {
			"ns",
			"us",
			"ms",
			"s",
		};
		auto textTimeNs = [&](double time)
		{
			if (std::isnan(time))
			{
				_strings.pushBack("?", false);
			}
			else
			{
				uint32_t unit_id = 0;
				while (unit_id < (units.size() - 1))
				{
					if (time < 1e3)
					{
						break;
					}
					time /= 1e3;
					++unit_id;
				}
				assert(unit_id < units.size());
				_strings.print(false, "%.2f%s", time, units[unit_id]);
			}
		};
		for (size_t i = 0; i < _stack.size(); ++i)
		{
			const size_t old_size = _strings.size();
			Segment & seg = _stack[i];

			const std::chrono::nanoseconds cpu_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(seg.end_timepoint - seg.begin_timepoint);
			const double cpu_duration_ns = cpu_duration.count();
			
			const double gpu_duration_ns = [&]()
			{
				if (seg.end_timestamp != uint64_t(-1))
				{
					const uint64_t diff = seg.end_timestamp - seg.begin_timestamp;
					return double(diff) * device_period;
				}
				else
				{
					return std::numeric_limits<double>::quiet_NaN();
				}
			}();
			const std::string_view label = _strings.get(_stack[i].label_range);
			
			_strings.print(false, "%s: ", label.data());
			textTimeNs(cpu_duration_ns);
			_strings.print(false, " | ");
			textTimeNs(gpu_duration_ns);
			_strings.pushNull();
			const size_t new_size = _strings.size();
			const uint32_t len = static_cast<uint32_t>((new_size - old_size) - 1);

			_stack[i].label_range = Range{.begin = static_cast<uint32_t>(old_size), .len = len};
		}
	}

	void ExecutionStackReport::declareGUI(GuiContext& ctx)
	{
		ImGui::PushID(this);
		uint32_t d = 0;
		// ns per tick
		const double device_period = application()->deviceProperties().props2.properties.limits.timestampPeriod;
		size_t i = 0;
		while(i < _stack.size())
		{
			const Segment & seg = _stack[i];
			const std::string_view label = _strings.get(seg.label_range);
			if (seg.depth < d)
			{
				while (seg.depth != d)
				{
					ImGui::TreePop();
					ImGui::PopID();
					--d;
				}
			}
			else
			{
				assert(seg.depth == d || seg.depth == (d + 1));
			}
			d = seg.depth;
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
			if (i < (_stack.size() - 1))
			{
				if (_stack[i + 1].depth != (seg.depth + 1))
				{
					flags |= ImGuiTreeNodeFlags_Leaf;
				}
			}
			else if (i == (_stack.size() - 1))
			{
				flags |= ImGuiTreeNodeFlags_Leaf;
			}
			assert(*(label.data() + label.size()) == char(0));
			ImGui::PushID(seg.label_range.begin);
			const bool node_open = ImGui::TreeNodeEx(label.data(), flags);

			if (node_open)
			{
				if (flags & ImGuiTreeNodeFlags_Leaf)
				{
					ImGui::TreePop();
					ImGui::PopID();
				}
				++i;
			}
			else
			{
				ImGui::PopID();
				++i;
				while (i < _stack.size())
				{
					if (_stack[i].depth <= seg.depth)
					{
						break;
					}
					++i;
				}
			}
		}
		while (d > 0)
		{
			ImGui::TreePop();
			ImGui::PopID();
			--d;
		}
		ImGui::PopID();
	}
}