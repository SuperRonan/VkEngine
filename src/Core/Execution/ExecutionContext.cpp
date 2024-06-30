#include "ExecutionContext.hpp"
#include <cassert>
#include <random>

#include <Core/Execution/ExecutionStackReport.hpp>

namespace vkl
{
	RecordContext::RecordContext(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_graphics_bound_sets(DescriptorSetsTacker::CI{
			.app = ci.app, 
			.name = ci.name + "._gfx_bound_sets",
			.pipeline_binding = VK_PIPELINE_BIND_POINT_GRAPHICS,
		}),
		_compute_bound_sets(DescriptorSetsTacker::CI{
			.app = ci.app,
			.name = ci.name + +"._cmp_bound_sets",
			.pipeline_binding = VK_PIPELINE_BIND_POINT_COMPUTE,
		}),
		_ray_tracing_bound_sets(DescriptorSetsTacker::CI{
			.app = ci.app,
			.name = ci.name + +"._rtx_bound_sets",
			.pipeline_binding = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		})
	{}

	DescriptorSetsTacker& RecordContext::getBoundSets(VkPipelineBindPoint pipeline)
	{
		switch (pipeline)
		{
			case VK_PIPELINE_BIND_POINT_GRAPHICS:
				return _graphics_bound_sets;
			break;
			case VK_PIPELINE_BIND_POINT_COMPUTE:
				return _compute_bound_sets;
			break;
			default: //case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
				return _ray_tracing_bound_sets;
			break;
		}
	}

	ExecutionContext::ExecutionContext(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_command_buffer(ci.cmd),
		_resource_tid(ci.resource_tid),
		_graphics_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._gfx_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_GRAPHICS,
		}),
		_compute_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._cmp_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_COMPUTE,
		}),
		_ray_tracing_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._rtx_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		})
	{
		_can_push_vk_debug_label = application()->options().enable_command_buffer_labels;
	}

	void ExecutionContext::pushDebugLabel(std::string_view const& label, vec4 const& color, bool timestamp)
	{
		++_debug_stack_depth;
		if (_can_push_vk_debug_label)
		{
			assert(*(label.data() + label.size()) == char(0));
			VkDebugUtilsLabelEXT vk_label{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
				.pNext = nullptr,
				.pLabelName = label.data(),
				.color = {color.r, color.g, color.b, color.a},
			};
			application()->extFunctions()._vkCmdBeginDebugUtilsLabelEXT(*_command_buffer, &vk_label);
		}
		const uint32_t begin_timestamp = getNewTimestampIndex();
		const uint32_t end_timestamp = getNewTimestampIndex();
		if (_stack_report)
		{
			ExecutionStackReport::Segment & segment = _stack_report->push(label, ImVec4(color.r, color.g, color.b, color.a));
			if (timestamp)
			{
				segment.begin_timestamp = begin_timestamp;
				segment.end_timestamp = end_timestamp;
				if (_timestamp_query_pool && _timestamp_query_pool->count() > segment.begin_timestamp)
				{
					vkCmdWriteTimestamp2(*_command_buffer, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, *_timestamp_query_pool, segment.begin_timestamp);
				}
			}
			segment.begin_timepoint = _tick_tock.tock();
		}
	}

	void ExecutionContext::pushDebugLabel(std::string_view const& label, bool timestamp)
	{
		std::hash<std::string_view> hs;
		size_t seed = hs(label);
		auto rng = std::mt19937_64(seed);
		std::uniform_real_distribution<float> distrib(0, 1);
		vec4 color;
		color.r = distrib(rng);
		color.g = distrib(rng);
		color.b = distrib(rng);
		color.a = 1;
		pushDebugLabel(label, color, timestamp);
	}

	void ExecutionContext::popDebugLabel()
	{
		assert(_debug_stack_depth != 0);
		--_debug_stack_depth;
		if (_stack_report)
		{
			ExecutionStackReport::Segment * to_pop = _stack_report->getStackTop();
			assert(to_pop);
			if (to_pop->end_timestamp != uint64_t(-1))
			{
				if (_timestamp_query_pool && _timestamp_query_pool->count() > to_pop->end_timestamp)
				{
					vkCmdWriteTimestamp2(*_command_buffer, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, *_timestamp_query_pool, to_pop->end_timestamp);
				}
			}
			to_pop->end_timepoint = _tick_tock.tock();
			_stack_report->pop();
		}
		if (_can_push_vk_debug_label)
		{
			application()->extFunctions()._vkCmdEndDebugUtilsLabelEXT(*_command_buffer);
		}
	}

	void ExecutionContext::insertDebugLabel(std::string_view const& label, vec4 const& color)
	{
		if (_can_push_vk_debug_label)
		{
			assert(label[label.size()] == char(0));
			VkDebugUtilsLabelEXT vk_label{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
				.pNext = nullptr,
				.pLabelName = label.data(),
				.color = {color.r, color.g, color.b, color.a},
			};
			application()->extFunctions()._vkCmdInsertDebugUtilsLabelEXT(*_command_buffer, &vk_label);
		}
	}
}
