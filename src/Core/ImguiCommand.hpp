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

		std::vector<std::shared_ptr<ImageView>> _targets = {};
		std::vector<std::shared_ptr<Framebuffer>> _framebuffers = {};

		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

		size_t _index = 0;

		void createRenderPass();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<std::shared_ptr<ImageView>> targets = {};
		};
		using CI = CreateInfo;

		ImguiCommand(CreateInfo const& ci);

		virtual void init()override;

		virtual void execute(ExecutionContext& context) override;

		
		virtual ~ImguiCommand() override;


		constexpr void setIndex(size_t index)
		{
			_index = index;
		}
	};
}