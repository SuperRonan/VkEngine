#pragma once

#include "DeviceCommand.hpp"
#include <utility>
#include <cassert>
#include "ShaderBindingDescriptor.hpp"

#include <Core/Execution/DescriptorSetsManager.hpp>

namespace vkl
{	

	class ShaderCommandNode : public ExecutionNode
	{
	public:
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

		std::shared_ptr<DescriptorSetAndPoolInstance> _set;
		std::shared_ptr<PipelineInstance> _pipeline;
		MyVector<std::shared_ptr<ImageViewInstance>> _image_views_to_keep;

		virtual void clear() override;

		virtual void recordPushConstant(CommandBuffer& cmd, ExecutionContext& ctx, PushConstant const& pc);

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

	};
}