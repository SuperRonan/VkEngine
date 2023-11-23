#pragma once

#include <Core/Execution/ExecutionContext.hpp>
#include <functional>
#include <cassert>

namespace vkl
{
	class ExecutionNode
	{
	public:

		using ExecFn = std::function<void(ExecutionContext&)>;

	protected:

		std::string _name = {};
		std::vector<Resource> _resources = {};
		ExecFn _exec_fn = {};

	public:
		
		struct CreateInfo
		{
			std::string name = {};
			std::vector<Resource> resources = {};
			ExecFn exec_fn = {};
		};
		using CI = CreateInfo;

		ExecutionNode(CreateInfo const& ci):
			_name(ci.name),
			_resources(ci.resources),
			_exec_fn(ci.exec_fn)
		{}

		constexpr const std::string& name()const
		{
			return _name;
		}

		constexpr const std::vector<Resource> & resources()const
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