#pragma once

#include <Core/Execution/ExecutionContext.hpp>
#include <functional>
#include <cassert>
#include <iterator>

namespace vkl
{
	class ExecutionNode
	{
	public:

		using ExecFn = std::function<void(ExecutionContext&)>;

	protected:

		std::string _name = {};
		std::vector<ResourceInstance> _resources = {};
		ExecFn _exec_fn = {};

	public:
		
		struct CreateInfo
		{
			std::string name = {};
			std::vector<ResourceInstance> resources = {};
			ExecFn exec_fn = {};
		};
		using CI = CreateInfo;

		ExecutionNode(CreateInfo const& ci):
			_name(ci.name),
			_resources(ci.resources),
			_exec_fn(ci.exec_fn)
		{}

		void addResource(ResourceInstance const& ri)
		{
			_resources.push_back(ri);
		}

		void addResource(ResourceInstance && ri)
		{
			_resources.emplace_back(ri);
		}

		template <class ResourceInstanceIt>
		void addResources(ResourceInstanceIt begin, ResourceInstanceIt const& end)
		{	
			using iter_category = typename std::iterator_traits<ResourceInstanceIt>::iterator_category;
			if constexpr (std::is_same<iter_category, std::random_access_iterator_tag>::value)
			{
				const size_t N = std::distance(begin, end);
				const size_t o = _resources.size();
				_resources.resize(o + N);
				for (size_t i = 0; i < N; ++i)
				{
					_resources[o + i] = *(begin + i);
				}
			}
			else
			{
				while (begin != end)
				{
					addResource(*begin);
					++begin;
				}
			}
		}

		constexpr const std::string& name()const
		{
			return _name;
		}

		constexpr const std::vector<ResourceInstance> & resources()const
		{
			return _resources;
		}

		void run(ExecutionContext& ctx) const
		{
			assert(_exec_fn);
			_exec_fn(ctx);
		}
	};

	class Command : public VkObject
	{
	protected:

	public:

		template <typename StringLike>
		constexpr Command(VkApplication* app, StringLike&& name = "") :
			VkObject(app, std::forward<StringLike>(name))
		{}

		virtual ~Command() = default;

		virtual void init() {};

		virtual ExecutionNode getExecutionNode(RecordContext & ctx) = 0;

		ExecutionNode operator()(RecordContext & ctx)
		{
			return getExecutionNode(ctx);
		}

		virtual bool updateResources(UpdateContext & ctx) 
		{ 
			return false; 
		};

	};

	using Executable = std::function<ExecutionNode(RecordContext& ctx)>;
}