#pragma once

#include <Core/Execution/ResourceBinding.hpp>
#include <Core/VkObjects/Program.hpp>
#include <Core/VkObjects/DescriptorPool.hpp>
#include <Core/VkObjects/DescriptorSet.hpp>
#include <Core/VkObjects/CommandBuffer.hpp>

namespace vkl
{
	using Binding = ShaderBindingDescription;
	using SetRange = Range32u;

	class DescriptorSetAndPoolInstance : public AbstractInstance
	{
	protected:
		
		std::shared_ptr<DescriptorSetLayoutInstance> _layout = nullptr;

		// sorted and at the size of layout.bindings -> can keep a pointer on a binding
		ResourceBindings _bindings = {};

		std::shared_ptr<DescriptorPool> _pool = nullptr;
		std::shared_ptr<DescriptorSet> _set = nullptr;

		bool _allow_null_bindings = true;

		size_t findBindingIndex(uint32_t b) const;

		void sortBindings();

		void installInvalidationCallback(ResourceBinding & binding, Callback & cb);

		void removeInvalidationCallbacks(ResourceBinding & binding);

		void installInvalidationCallbacks();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<DescriptorSetLayoutInstance> layout = nullptr;
			ResourceBindings bindings = {}; // Not sorted
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

		void setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const BufferAndRange* buffers = nullptr);

		void setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const std::shared_ptr<ImageView>* views = nullptr, const std::shared_ptr<Sampler>* samplers = nullptr);

		//void setBinding(ResourceBinding const& b);
		
	};

	class DescriptorSetAndPool : public InstanceHolder<DescriptorSetAndPoolInstance>
	{
	protected:

		using ParentType = InstanceHolder<DescriptorSetAndPoolInstance>;

		std::shared_ptr<DescriptorSetLayout> _layout = nullptr;
		bool _layout_from_prog = false;

		std::shared_ptr<Program> _prog = nullptr;
		uint32_t _target_set = -1;

		bool _allow_missing_bindings = true;

		// Not sorted
		ResourceBindings _bindings = {};

		std::shared_ptr<AsynchTask> _create_instance_task = nullptr;

		void destroyInstance();

		ResourceBindings resolveBindings(AsynchTask::ReturnType& result);

		ResourceBindings::iterator findBinding(uint32_t b);

		ResourceBinding * findBindingOrEmplace(uint32_t b);
		
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

		//void setBinding(ShaderBindingDescription const& binding);

		struct Registration
		{
			std::shared_ptr<DescriptorSetAndPool> set;
			uint32_t binding;
			uint32_t array_index;

			void clear(uint32_t num_bindings);
		};

		void clearBinding(uint32_t binding, uint32_t array_index, uint32_t count);

		void setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const BufferAndRange * buffers = nullptr);

		void setBinding(uint32_t binding, uint32_t array_index, uint32_t count, const std::shared_ptr<ImageView> * views = nullptr, const std::shared_ptr<Sampler> * samplers = nullptr);

		void waitForInstanceCreationIFN();
	};


	class DescriptorSetsTacker : public VkObject
	{
	protected:
		VkPipelineBindPoint _pipeline_binding;
		std::vector<std::shared_ptr<DescriptorSetAndPoolInstance>> _bound_descriptor_sets;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			VkPipelineBindPoint pipeline_binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		};
		using CI = CreateInfo;

		DescriptorSetsTacker(CreateInfo const& ci);

		void bind(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set);

		const std::shared_ptr<DescriptorSetAndPoolInstance>& getSet(uint32_t s) const;
	};


	class DescriptorSetsManager : public DescriptorSetsTacker
	{
	protected:

		std::shared_ptr<CommandBuffer> _cmd;

		std::vector<Range32u> _bindings_ranges;

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

		void recordBinding(std::shared_ptr<PipelineLayoutInstance> const& layout, PerBindingFunction const& func = nullptr);

		void bindOneAndRecord(uint32_t binding, std::shared_ptr<DescriptorSetAndPoolInstance> const& set, std::shared_ptr<PipelineLayoutInstance> const& layout);
	};
}