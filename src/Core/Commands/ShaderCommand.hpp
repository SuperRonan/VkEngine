#pragma once

#include "DeviceCommand.hpp"
#include <utility>
#include <cassert>
#include "ShaderBindingDescriptor.hpp"
#include <Core/Execution/ResourceBinding.hpp>

namespace vkl
{	
	using Binding = ShaderBindingDescription;

	using PushConstant = ObjectView;

	class DescriptorSetsInstance : public AbstractInstance
	{
	protected:
		
		using SetRange = Range32;

		std::shared_ptr<ProgramInstance> _prog;
		std::vector<ResourceBinding> _bindings;
		std::vector<std::shared_ptr<DescriptorPool>> _desc_pools;
		
		std::vector<std::shared_ptr<DescriptorSet>> _desc_sets;
		std::vector<SetRange> _set_ranges;
		
		std::vector<Resource> _resources;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<ResourceBinding> bindings;
			std::shared_ptr<ProgramInstance> program = nullptr;
		};
		using CI = CreateInfo;

		DescriptorSetsInstance(CreateInfo const& ci);

		virtual ~DescriptorSetsInstance() override;

		void allocateDescriptorSets();

		void resolveBindings();

		void writeDescriptorSets();

		void recordBindings(CommandBuffer& cmd, VkPipelineBindPoint binding);

		void recordInputSynchronization(SynchronizationHelper& synch);

	};

	class DescriptorSetsManager : public InstanceHolder<DescriptorSetsInstance>
	{
	protected:
		using ParentType = InstanceHolder<DescriptorSetsInstance>;
		
		std::shared_ptr<Program> _prog;
		std::vector<ResourceBinding> _bindings;
	
		void createInstance(ShaderBindings const& common_bindings);

		void destroyInstance();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::shared_ptr<Program> program = nullptr;
		};
		using CI = CreateInfo;

		DescriptorSetsManager(CreateInfo const& ci);

		virtual ~DescriptorSetsManager() override;

		bool updateResources(UpdateContext & ctx);
	};

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