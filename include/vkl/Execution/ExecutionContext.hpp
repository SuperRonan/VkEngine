#pragma once

#include <vkl/App/VkApplication.hpp>

#include <vkl/VkObjects/CommandBuffer.hpp>
#include <vkl/VkObjects/Pipeline.hpp>
#include <vkl/VkObjects/DescriptorSet.hpp>
#include <vkl/VkObjects/Buffer.hpp>
#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Sampler.hpp>
#include <vkl/VkObjects/RenderPass.hpp>
#include <vkl/VkObjects/Framebuffer.hpp>
#include <vkl/VkObjects/QueryPool.hpp>

#include <vkl/Execution/BufferPool.hpp>
#include <vkl/Execution/ResourceState.hpp>
#include <vkl/Execution/DescriptorSetsManager.hpp>
#include <vkl/Execution/FramePerformanceCounters.hpp>

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <stack>

namespace vkl
{
	class ExecutionStackReport;

	class RecordContext : public VkObject
	{
	protected:

		DescriptorSetsTacker _graphics_bound_sets;
		DescriptorSetsTacker _compute_bound_sets;
		DescriptorSetsTacker _ray_tracing_bound_sets;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name;
		};
		using CI = CreateInfo;

		RecordContext(CreateInfo const& ci);

		DescriptorSetsTacker& graphicsBoundSets()
		{
			return _graphics_bound_sets;
		}

		DescriptorSetsTacker& computeBoundSets()
		{
			return _graphics_bound_sets;
		}

		DescriptorSetsTacker& rayTracingBoundSets()
		{
			return _graphics_bound_sets;
		}

		DescriptorSetsTacker& getBoundSets(VkPipelineBindPoint pipeline);
	};

	class ExecutionContext : public VkObject
	{
	protected:

		std::TickTock_hrc _tick_tock;
		using TimePoint = decltype(_tick_tock)::TimePoint;
		using Duration = decltype(_tick_tock)::Duration;

		std::shared_ptr<CommandBuffer> _command_buffer = nullptr;

		uint32_t _debug_stack_depth = 0;
		uint32_t _timestamp_query_count = 0;
		std::shared_ptr<QueryPoolInstance> _timestamp_query_pool = nullptr;

		uint32_t getNewTimestampIndex()
		{
			uint32_t res = _timestamp_query_count;
			++_timestamp_query_count;
			return res;
		}

		using vec4 = glm::vec4;

		std::shared_ptr<ExecutionStackReport> _stack_report;

		bool _can_push_vk_debug_label = false;

		size_t _resource_tid = 0;

		MyVector<std::shared_ptr<VkObject>> _objects_to_keep = {};
		MyVector<CompletionCallback> _completion_callbacks = {};

		DescriptorSetsManager _graphics_bound_sets;
		DescriptorSetsManager _compute_bound_sets;
		DescriptorSetsManager _ray_tracing_bound_sets;

		FramePerfCounters * _frame_perf_counters = nullptr;

		friend class LinearExecutor;


	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<CommandBuffer> cmd = nullptr;
			size_t resource_tid = 0;
		};
		using CI = CreateInfo;

		ExecutionContext(CreateInfo const& ci);

		constexpr std::shared_ptr<CommandBuffer>& getCommandBuffer()
		{
			return _command_buffer;
		}

		void setCommandBuffer(std::shared_ptr<CommandBuffer> cmd)
		{
			_command_buffer = cmd;
			_graphics_bound_sets.setCommandBuffer(_command_buffer);
			_compute_bound_sets.setCommandBuffer(_command_buffer);
			_ray_tracing_bound_sets.setCommandBuffer(_command_buffer);
		}

		void keepAlive(std::shared_ptr<VkObject> const& obj)
		{
			_objects_to_keep.emplace_back(obj);
		}

		void keepAlive(std::shared_ptr<VkObject> && obj)
		{
			_objects_to_keep.emplace_back(std::move(obj));
		}

		template <std::derived_from<VkObject> O>
		void keepAlive(MyVector<std::shared_ptr<O>> const& objs)
		{
			_objects_to_keep += objs;
		}

		auto& objectsToKeepAlive()
		{
			return _objects_to_keep;
		}

		void addCompletionCallback(CompletionCallback const& cb)
		{
			_completion_callbacks.push_back(cb);
		}

		void addCompletionCallback(CompletionCallback && cb)
		{
			_completion_callbacks.push_back(std::move(cb));
		}

		MyVector<CompletionCallback> const& completionCallbacks() const
		{
			return _completion_callbacks;
		}

		MyVector<CompletionCallback> & completionCallbacks()
		{
			return _completion_callbacks;
		}

		const auto& objectsToKeepAlive() const
		{
			return _objects_to_keep;
		}

		size_t resourceThreadId()const
		{
			return  _resource_tid;
		}

		DescriptorSetsManager& graphicsBoundSets()
		{
			return _graphics_bound_sets;
		}

		DescriptorSetsManager& computeBoundSets()
		{
			return _compute_bound_sets;
		}

		DescriptorSetsManager& rayTracingBoundSets()
		{
			return _ray_tracing_bound_sets;
		}

		void setFramePerfCounters(FramePerfCounters* fpc)
		{
			_frame_perf_counters = fpc;
		}

		FramePerfCounters* framePerfCounters() const
		{
			return _frame_perf_counters;
		}
		

		void pushDebugLabel(std::string_view const& label, vec4 const& color, bool timestamp = false);

		void pushDebugLabel(std::string_view const& label, bool timestamp = false);

		void popDebugLabel();

		void insertDebugLabel(std::string_view const& label, vec4 const& color);
	};
}

