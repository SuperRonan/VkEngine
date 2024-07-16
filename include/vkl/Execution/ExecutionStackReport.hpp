#pragma once

#include <vkl/App/VkApplication.hpp>

#include <vkl/IO/GuiContext.hpp>

#include <vkl/Execution/Module.hpp>

#include <vkl/VkObjects/QueryPool.hpp>

#include <that/utils/ExtensibleStringStorage.hpp>

namespace vkl
{
	class ExecutionStackReport : public Module
	{
	public:

		// Note: 
		// This object has 2 stages
		// First stage: accumulation of data
		// Segment::timestamp* are timestamp query indices (-1 means no value), could be uint32_t

		// Second stage: display of data
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


		MyVector<Segment> _stack = {};
		Index _stack_top = Index(-1);

		that::ExtensibleStringStorage _strings;
		
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