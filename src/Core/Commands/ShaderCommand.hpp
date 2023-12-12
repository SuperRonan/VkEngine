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

		MultiDescriptorSetsLayouts _provided_sets_layouts = {};

		std::shared_ptr<DescriptorSetAndPool> _set;

		std::shared_ptr<Pipeline> _pipeline;

		PushConstant _pc;

		ResourcesInstances getDescriptorSetResources(DescriptorSetAndPoolInstance & set, DescriptorSetLayout const& layout);

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
		};

		ShaderCommand(CreateInfo const& ci) :
			DeviceCommand(ci.app, ci.name),
			_provided_sets_layouts(ci.sets_layouts)
		{

		}

		virtual ~ShaderCommand() override = default;

		virtual void recordPushConstant(CommandBuffer& cmd, ExecutionContext& ctx, PushConstant const& pc);

		virtual void recordBindings(CommandBuffer& cmd, ExecutionContext& context);

		virtual ResourcesInstances getBoundResources(DescriptorSetsTacker & bound_sets, size_t max_set=0);

		template<typename T>
		void setPushConstantsData(T && t)
		{
			_pc = t;
		}

		virtual void init() override 
		{ 
			DeviceCommand::init();
		};

		virtual bool updateResources(UpdateContext & ctx) override;

	};
}