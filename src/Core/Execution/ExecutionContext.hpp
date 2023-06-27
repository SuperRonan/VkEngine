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

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace vkl
{
	class ExecutionContext
	{
	protected:

		std::shared_ptr<CommandBuffer> _command_buffer = nullptr;

		size_t _resource_tid = 0;

		std::vector<std::shared_ptr<VkObject>> _objects_to_keep = {};

		StagingPool* _staging_pool = nullptr;


		friend class LinearExecutor;


	public:

		struct CreateInfo
		{
			std::shared_ptr<CommandBuffer> cmd = nullptr;
			size_t resource_tid = 0;
			StagingPool* staging_pool = nullptr;
		};
		using CI = CreateInfo;

		ExecutionContext(CreateInfo const& ci);

		constexpr std::shared_ptr<CommandBuffer>& getCommandBuffer()
		{
			return _command_buffer;
		}

		void setCommandBuffer(std::shared_ptr<CommandBuffer> cmd);

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

