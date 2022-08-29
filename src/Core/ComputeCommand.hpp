#pragma once

#include "ShaderCommand.hpp"

namespace vkl
{
	class ComputeCommand : public ShaderCommand
	{
	protected:

		std::shared_ptr<ComputeProgram> _program;
		VkExtent3D _dispatch_size;
		bool _dispatch_threads = false;

	public:

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context) override;

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;
	};
}