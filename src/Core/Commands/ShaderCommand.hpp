#pragma once

#include "DeviceCommand.hpp"
#include <utility>
#include <cassert>
#include "ShaderBindingDescriptor.hpp"

#include <Core/Execution/DescriptorSetsManager.hpp>

namespace vkl
{	
	using PushConstant = ObjectView;

	class ShaderCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<DescriptorSetsManager> _sets;

		std::shared_ptr<Pipeline> _pipeline;

		PushConstant _pc;

	public:

		template <typename StringLike = std::string>
		ShaderCommand(VkApplication* app, StringLike&& name, std::vector<ShaderBindingDescription> const& bindings) :
			DeviceCommand(app, std::forward<StringLike>(name))
		{

		}

		virtual ~ShaderCommand() override = default;

		virtual void recordPushConstant(CommandBuffer& cmd, ExecutionContext& ctx, PushConstant const& pc);

		virtual void recordBindings(CommandBuffer& cmd, ExecutionContext& context);

		template<typename T>
		void setPushConstantsData(T && t)
		{
			_pc = t;
		}

		virtual void init() override 
		{ 
			DeviceCommand::init();
		};

		virtual void execute(ExecutionContext& context) override = 0;

		virtual bool updateResources(UpdateContext & ctx) override;

	};
}