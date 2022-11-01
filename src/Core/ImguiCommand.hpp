#pragma once

#include <Core/ExecutionContext.hpp>
#include <Core/DeviceCommand.hpp>
#include <Core/RenderPass.hpp>
#include <Core/DescriptorPool.hpp>

namespace vkl
{
	class ImguiCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<RenderPass> _render_pass;
		std::shared_ptr<DescriptorPool> _desc_pool;

	public:

		virtual void init()override;

		virtual void execute(ExecutionContext& context) override;

	};
}