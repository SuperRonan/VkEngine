#pragma once

#include <vkl/Execution/ExecutionContext.hpp>
#include <vkl/Commands/ResourceUsageList.hpp>

#include <functional>
#include <cassert>
#include <iterator>
#include <atomic>
#include <mutex>



namespace vkl
{
	// Maybe make clear that the node is single use (can call execute once)
	// 
	class ExecutionNode : public VkObject
	{
	public:

	protected:

		//bool _is_use = false;
		std::atomic<bool> _in_use = false;
		ResourceUsageList _resources = {};

	public:
		
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		ExecutionNode(CreateInfo const& ci):
			VkObject(ci.app, ci.name)
		{}

		void setInUse(bool usage = true)
		{
			_in_use = usage;
		}

		bool isInUse()
		{
			return _in_use;
		}
		
		void finish()
		{
			setInUse(false);
			clear();
		}

		virtual void clear();

		virtual void execute(ExecutionContext& ctx)
		{}

		ResourceUsageList& resources()
		{
			return _resources;
		}

		const ResourceUsageList& resources() const
		{
			return _resources;
		}
	};

	class ExecutionNodePool
	{
	protected:
		
		std::mutex _mutex;
		MyVector<std::shared_ptr<ExecutionNode>> _available_nodes;
		std::deque<std::shared_ptr<ExecutionNode>> _in_use_nodes;

		void recycleNodes();

	public:

		template <class CreateNodeFn>
		std::shared_ptr<ExecutionNode> getCleanNode(CreateNodeFn const& create_node_fn)
		{
			std::shared_ptr<ExecutionNode> res;
			std::unique_lock lock(_mutex);

			recycleNodes();

			if (_available_nodes)
			{
				res = _available_nodes.back();
				_available_nodes.pop_back();
				// Assume res is already cleared
			}
			else
			{
				res = create_node_fn();
			}
			assert(!!res);
			res->setInUse();
			_in_use_nodes.push_back(res);
			return res;
		}

		template <std::derived_from<ExecutionNode> DerivedExecutionNode, class CreateNodeFn>
		std::shared_ptr<DerivedExecutionNode> getCleanNode(CreateNodeFn const& create_node_fn)
		{
			std::shared_ptr<ExecutionNode> res = getCleanNode(create_node_fn);
			return std::dynamic_pointer_cast<DerivedExecutionNode>(res);
		}
	};

	class Command : public VkObject
	{
	protected:
		
		ExecutionNodePool _exec_node_cache;

	public:

		template <typename StringLike>
		constexpr Command(VkApplication* app, StringLike&& name = "") :
			VkObject(app, std::forward<StringLike>(name))
		{}

		virtual ~Command() = default;

		virtual void init() {};

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) = 0;

		std::shared_ptr<ExecutionNode> operator()(RecordContext & ctx)
		{
			return getExecutionNode(ctx);
		}

		virtual bool updateResources(UpdateContext & ctx) 
		{ 
			return false;
		};

	};

	using Executable = std::function<std::shared_ptr<ExecutionNode>(RecordContext& ctx)>;
}