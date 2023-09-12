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
	using SetRange = Range32;

	class DescriptorSetAndPoolInstance : public AbstractInstance
	{
	protected:
		
		// Can be nullptr
		std::shared_ptr<ProgramInstance> _prog = nullptr;
		uint32_t _target_set = -1;

		// sorted
		ResourceBindings _bindings = {};

		std::shared_ptr<DescriptorPool> _pool = nullptr;
		std::shared_ptr<DescriptorSet> _set = nullptr;

		size_t findBindingIndex(uint32_t b) const;

		void sortBindings();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			const DescriptorSetBindingGlobalOptions& options;
			VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_MAX_ENUM;
			std::shared_ptr<ProgramInstance> progam = nullptr;
			uint32_t target_set = -1;
			ResourceBindings bindings = {};
		};
		using CI = CreateInfo;

		DescriptorSetAndPoolInstance(CreateInfo const& ci);

		virtual ~DescriptorSetAndPoolInstance() override;

		bool checkIntegrity()const;

		void allocateDescriptorSet();

		void resolveBindings();

		void writeDescriptorSet();


		constexpr const std::shared_ptr<DescriptorPool> & pool()const
		{
			return _pool;
		}

		constexpr const std::shared_ptr<DescriptorSet>& set()const
		{
			return _set;
		}

		ResourceBindings& bindings()
		{
			return _bindings;
		}

		const ResourceBindings& bindings() const
		{
			return _bindings;
		}

		ResourceBinding* findBinding(uint32_t b)
		{
			const size_t index = findBindingIndex(b);
			if (index == size_t(-1))
			{
				return nullptr;
			}
			else
			{
				return _bindings.data() + index;
			}
		}
		
	};

	class DescriptorSetAndPool : public InstanceHolder<DescriptorSetAndPoolInstance>
	{
	protected:

		using ParentType = InstanceHolder<DescriptorSetAndPoolInstance>;

		const DescriptorSetBindingGlobalOptions& _options;

		std::shared_ptr<Program> _prog = nullptr;
		uint32_t _target_set = -1;

		ResourceBindings _bindings = {};
		
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			const DescriptorSetBindingGlobalOptions& options;
			std::shared_ptr<Program> progam = nullptr;
			uint32_t target_set = -1;
			ShaderBindings bindings = {};
		};
		using CI = CreateInfo;

		DescriptorSetAndPool(CreateInfo const& ci);

		bool updateResources();

	};

	class DescriptorSetsInstance : public AbstractInstance
	{
	protected:

		std::shared_ptr<ProgramInstance> _prog;
		std::vector<ResourceBinding> _bindings;
		std::vector<std::shared_ptr<DescriptorPool>> _desc_pools;
		
		std::vector<std::shared_ptr<DescriptorSet>> _desc_sets;
		std::vector<SetRange> _set_ranges;
	

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

	class DescriptorSets : public InstanceHolder<DescriptorSetsInstance>
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

		DescriptorSets(CreateInfo const& ci);

		virtual ~DescriptorSets() override;

		bool updateResources(UpdateContext & ctx);
	};

	class DescriptorSetsManager : public VkObject
	{
	protected:

		std::shared_ptr<CommandBuffer> _cmd;

		VkPipelineBindPoint _pipeline_binding;
		
		std::vector<std::shared_ptr<DescriptorSetAndPoolInstance>> _bound_descriptor_sets;

		std::vector<Range32> _bindings_ranges;

		std::vector<VkDescriptorSet> _vk_sets;


	public:


		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<CommandBuffer> cmd = nullptr;
			VkPipelineBindPoint pipeline_binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		};
		using CI = CreateInfo;

		DescriptorSetsManager(CreateInfo const& ci);

		void bind(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set);

		void recordBinding(std::shared_ptr<PipelineLayout> const& layout);

		const std::shared_ptr<DescriptorSetAndPoolInstance> & getSet(uint32_t s) const;
	};
}