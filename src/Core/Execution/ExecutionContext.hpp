#pragma once

#include <Core/App/VkApplication.hpp>

#include <Core/VkObjects/CommandBuffer.hpp>
#include <Core/VkObjects/Pipeline.hpp>
#include <Core/VkObjects/DescriptorSet.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Sampler.hpp>
#include <Core/VkObjects/RenderPass.hpp>
#include <Core/VkObjects/Framebuffer.hpp>

#include <Core/Execution/BufferPool.hpp>
#include <Core/Execution/ResourceState.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Execution/FramePerformanceCounters.hpp>

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <stack>

namespace vkl
{
	class RecordContext : public VkObject
	{
	protected:

		DescriptorSetsTacker _graphics_bound_sets;
		DescriptorSetsTacker _compute_bound_sets;
		DescriptorSetsTacker _ray_tracing_bound_sets;

		BufferPool * _staging_pool = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name;
			BufferPool * staging_pool = nullptr;
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

		BufferPool* stagingPool()const
		{
			return _staging_pool;
		}
	};

	class ExecutionContext : public VkObject
	{
	protected:

		std::shared_ptr<CommandBuffer> _command_buffer = nullptr;

		using vec4 = glm::vec4;
		struct DebugLabel {
			std::string label;
			vec4 color;
		};
		std::stack<DebugLabel> _debug_labels;
		bool _can_push_vk_debug_label = false;

		size_t _resource_tid = 0;

		MyVector<std::shared_ptr<VkObject>> _objects_to_keep = {};

		BufferPool* _staging_pool = nullptr;

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
			BufferPool* staging_pool = nullptr;
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

		const auto& objectsToKeepAlive() const
		{
			return _objects_to_keep;
		}

		BufferPool* stagingPool()
		{
			return _staging_pool;
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
		

		void pushDebugLabel(std::string const& label, vec4 const& color);

		void pushDebugLabel(std::string const& label);

		void popDebugLabel();

		void insertDebugLabel(std::string const& label, vec4 const& color);
	};
}

