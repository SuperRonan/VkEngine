#pragma once

#include <Core/Execution/ResourceBinding.hpp>
#include <Core/VkObjects/Program.hpp>
#include <Core/VkObjects/DescriptorPool.hpp>
#include <Core/VkObjects/DescriptorSet.hpp>
#include <Core/VkObjects/CommandBuffer.hpp>

namespace vkl
{
	using Binding = ShaderBindingDescription;
	using SetRange = Range32;

	class DescriptorSetAndPoolInstance : public AbstractInstance
	{
	protected:
		
		std::shared_ptr<DescriptorSetLayout> _layout = nullptr;

		// sorted
		ResourceBindings _bindings = {};

		std::shared_ptr<DescriptorPool> _pool = nullptr;
		std::shared_ptr<DescriptorSet> _set = nullptr;

		size_t findBindingIndex(uint32_t b) const;

		void sortBindings();

		void installInvalidationCallbacks();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<DescriptorSetLayout> layout = nullptr;
			ResourceBindings bindings = {};
		};
		using CI = CreateInfo;

		DescriptorSetAndPoolInstance(CreateInfo const& ci);

		virtual ~DescriptorSetAndPoolInstance() override;

		bool checkIntegrity()const;

		void allocateDescriptorSet();

		void writeDescriptorSet(UpdateContext * context);

		bool exists() const
		{
			return !!_layout;
		}

		bool empty() const
		{
			return _bindings.empty();
		}


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

		std::shared_ptr<DescriptorSetLayout> _layout = nullptr;
		bool _layout_from_prog = false;

		std::shared_ptr<Program> _prog = nullptr;
		uint32_t _target_set = -1;

		ResourceBindings _bindings = {};

		void createInstance();

		void destroyInstance();

		ResourceBindings resolveBindings();
		
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<DescriptorSetLayout> layout = nullptr;
			std::shared_ptr<Program> program = nullptr;
			uint32_t target_set = uint32_t(-1);
			ShaderBindings bindings = {};
		};
		using CI = CreateInfo;

		DescriptorSetAndPool(CreateInfo const& ci);

		virtual ~DescriptorSetAndPool() override;

		bool updateResources(UpdateContext & context);

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

		void setCommandBuffer(std::shared_ptr<CommandBuffer> const& cmd);

		void bind(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set);

		using PerBindingFunction = std::function<void(std::shared_ptr<DescriptorSetAndPoolInstance>)>;

		void recordBinding(std::shared_ptr<PipelineLayout> const& layout, PerBindingFunction const& func = nullptr);

		const std::shared_ptr<DescriptorSetAndPoolInstance> & getSet(uint32_t s) const;
	};
}