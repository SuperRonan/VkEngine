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

		std::shared_ptr<ImageView> _target;

		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> target = nullptr;
		};
		using CI = CreateInfo;

		ImguiCommand(CreateInfo const& ci);

		virtual void init()override;

		virtual void execute(ExecutionContext& context) override;

	};
}