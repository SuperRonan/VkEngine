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
#include <Core/Rendering/Mesh.hpp>
#include <Core/Execution/StagingPool.hpp>
#include <Core/Execution/ResourceState.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace vkl
{
	class ExecutionContext : public VkObject
	{
	protected:

		std::shared_ptr<CommandBuffer> _command_buffer = nullptr;

		size_t _resource_tid = 0;

		std::vector<std::shared_ptr<VkObject>> _objects_to_keep = {};

		StagingPool* _staging_pool = nullptr;

		MountingPoints * _mounting_points = nullptr;

		DescriptorSetsManager _graphics_bound_sets;
		DescriptorSetsManager _compute_bound_sets;
		DescriptorSetsManager _ray_tracing_bound_sets;


		friend class LinearExecutor;


	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<CommandBuffer> cmd = nullptr;
			size_t resource_tid = 0;
			StagingPool* staging_pool = nullptr;
			MountingPoints * mounting_points = nullptr;
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

		void keppAlive(std::shared_ptr<VkObject> obj)
		{
			_objects_to_keep.emplace_back(std::move(obj));
		}

		auto& objectsToKeepAlive()
		{
			return _objects_to_keep;
		}

		const auto& objectsToKeepAlive() const
		{
			return _objects_to_keep;
		}

		StagingPool* stagingPool()
		{
			return _staging_pool;
		}

		size_t resourceThreadId()const
		{
			return  _resource_tid;
		}

		MountingPoints* mountingPoints()
		{
			return _mounting_points;
		}

		const MountingPoints* mountingPoints() const
		{
			return _mounting_points;
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
	};



	//class Executable
	//{
	//public:
	//	using ExecutableFunction = std::function<void(ExecutionContext& ctx)>;
	//protected:

	//	ExecutableFunction _function;

	//public:

	//	Executable(ExecutableFunction const& f):
	//		_function(f)
	//	{}

	//	void operator()(ExecutionContext& ctx)
	//	{
	//		_function(ctx);
	//	}
	//};
	using Executable = std::function<void(ExecutionContext& ctx)>;
}

