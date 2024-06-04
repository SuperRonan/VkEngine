#pragma once

#include <Core/App/VkApplication.hpp>

#include <Core/IO/GuiContext.hpp>

#include <Core/Execution/Module.hpp>

#include <Core/VkObjects/QueryPool.hpp>

#include <sstream>

namespace vkl
{
	class ExecutionStackReport : public Module
	{
	public:

		// Note: 
		// This object has 2 stages
		// First stage: accumulation of data
		// Segment::label point to just the label (stored in _string_buffer)
		// Segment::timestamp* are timestamp query indices (-1 means no value), could be uint32_t

		// Second stage: display of data
		// Segment::label points to a label with timings for the GUI (stored in _string_stream)
		// Segment::timestamp* now contains the retrieved timestamp value

		// Maybe encode the color in less space (sRGB + linear Alpha)
		using Color = ImVec4;
		using Clock = std::chrono::high_resolution_clock;
		using TimePoint = typename Clock::time_point;
		using Duration = typename Clock::duration;
		using Index = uint32_t;
		using Range = typename Range<Index>;

		struct Segment
		{
			Index depth = 0;
			Index parent = Index(-1);
			Range label_range = {};
			Color color = {};
			
			
			uint64_t begin_timestamp = uint64_t(-1);
			uint64_t end_timestamp = uint64_t(-1);

			TimePoint begin_timepoint = {};
			TimePoint end_timepoint = {};
		};
	
	protected:

		// Null terminated strings
		// At the begining for fast push labels
		MyVector<char> _string_buffer = {};
		MyVector<Segment> _stack = {};
		Index _stack_top = Index(-1);

		// a series of null terminated strings (needed for ImGui)
		std::stringstream _string_stream;
		
		TimePoint _begin_timepoint = {};
		TimePoint _end_timepoint = {};
		uint64_t _begin_timestamp = uint64_t(-1);
		uint64_t _end_timestamp = uint64_t(-1);

	public:
		
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		ExecutionStackReport(CreateInfo const& ci);

		virtual ~ExecutionStackReport() override = default;

		void clear();

		Segment& push(std::string_view label, Color const& color);

		Segment* getStackTop();
		
		void pop();

		std::string_view getStringView1(Range const& r) const
		{
			return std::string_view(_string_buffer.data() + r.begin, r.len);
		}

		std::string_view getStringView2(Range const& r) const
		{
			return std::string_view(_string_stream.view().data() + r.begin, r.len);
		}

		Range pushString(std::string_view sv);

		using GetQueryResultFn = std::function<void(uint32_t, uint64_t&, uint32_t*)>;
		void aquireQueryResults(GetQueryResultFn const& get_query_result_f);

		void prepareForGUI();

		TimePoint const& beginTimepoint() const
		{
			return _begin_timepoint;
		}

		TimePoint & beginTimepoint()
		{
			return _begin_timepoint;
		}

		void declareGUI(GuiContext& ctx);
	};
}