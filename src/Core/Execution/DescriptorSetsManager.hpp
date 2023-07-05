#pragma once

#include <Core/Execution/ResourceBinding.hpp>
#include <Core/VkObjects/Program.hpp>
#include <Core/VkObjects/DescriptorPool.hpp>
#include <Core/VkObjects/DescriptorSet.hpp>
#include <Core/VkObjects/CommandBuffer.hpp>
#include <Core/Execution/SynchronizationHelper.hpp>

namespace vkl
{
	using Binding = ShaderBindingDescription;

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
}