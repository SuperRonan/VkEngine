#pragma once

#include "DeviceCommand.hpp"
#include <utility>
#include <cassert>
#include "ShaderBindingDescriptor.hpp"

#include <vkl/Execution/DescriptorSetsManager.hpp>

#include <that/utils/ExtensibleDataStorage.hpp>
#include <that/utils/ExtensibleStringStorage.hpp>

namespace vkl
{	
	
	struct ShaderCommandList
	{
		using Index = size_t;
		using Range = Range<Index>;
		
		// For push constants
		that::ExDS _data;
		// For names
		that::ExSS _strings;

		Index pc_begin = 0;
		uint32_t pc_size = 0;
		uint32_t pc_offset = 0;

		void setPushConstant(const void * data, uint32_t size, uint32_t offset = 0);

		void clear();
	};

	class ShaderCommandNode : public ExecutionNode, public ShaderCommandList
	{
	public:
		
		using Range = ShaderCommandList::Range;

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		ShaderCommandNode(CreateInfo const& ci) :
			ExecutionNode(ExecutionNode::CI{
				.app = ci.app,
				.name = ci.name,
			})
		{}

		VkShaderStageFlags _pc_stages = 0;
		std::shared_ptr<DescriptorSetAndPoolInstance> _set;
		std::shared_ptr<PipelineInstance> _pipeline;
		MyVector<std::shared_ptr<ImageViewInstance>> _image_views_to_keep;

		virtual void clear() override;

		virtual void recordPushConstant(CommandBuffer& cmd, Index begin, uint32_t size, uint32_t offset = 0);

		void recordPushConstantIFN(CommandBuffer& cmd, Index begin, uint32_t size, uint32_t offset = 0)
		{
			if (size != 0)
			{
				recordPushConstant(cmd, begin, size, offset);
			}
		}

		virtual void recordBindings(CommandBuffer& cmd, ExecutionContext& context);
	};

	class ShaderCommand : public DeviceCommand
	{
	protected:

		MultiDescriptorSetsLayouts _provided_sets_layouts = {};

		std::shared_ptr<DescriptorSetAndPool> _set;

		std::shared_ptr<Pipeline> _pipeline;

		PushConstant _pc;

		void populateDescriptorSet(ShaderCommandNode & node, DescriptorSetAndPoolInstance & set, DescriptorSetLayoutInstance const& layout);

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
		};
		using CI = CreateInfo;

		ShaderCommand(CreateInfo const& ci);

		virtual ~ShaderCommand() override = default;

		
		// Pipeline and sets (up to shader)
		virtual void populateBoundResources(ShaderCommandNode & node, DescriptorSetsTacker & bound_sets, size_t max_set=0);

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

		std::shared_ptr<Pipeline> const& pipeline()const
		{
			return _pipeline;
		}

		std::shared_ptr<DescriptorSetAndPool> const& descriptorSet()const
		{
			return _set;
		}
	};
}