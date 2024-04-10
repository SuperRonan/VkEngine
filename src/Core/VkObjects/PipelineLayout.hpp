#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/VkObjects/DescriptorSetLayout.hpp>

namespace vkl
{
	class PipelineLayoutInstance : public AbstractInstance
	{
	protected:

		VkPipelineLayoutCreateFlags _flags = 0;
		std::vector<std::shared_ptr<DescriptorSetLayoutInstance>> _sets = {};
		std::vector<VkPushConstantRange> _push_constants = {};
		
		VkPipelineLayout _layout = VK_NULL_HANDLE;

		void create(VkPipelineLayoutCreateInfo const& ci);

		void setVkName();

		void destroy();
	
	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::vector<std::shared_ptr<DescriptorSetLayoutInstance>> sets = {};
			std::vector<VkPushConstantRange> push_constants = {};
		};
		using CI = CreateInfo;
		PipelineLayoutInstance(CreateInfo const& ci);

		virtual ~PipelineLayoutInstance() override;

		
		constexpr VkPipelineLayout layout()const
		{
			return _layout;
		}

		constexpr auto handle()const
		{
			return layout();
		}

		constexpr operator VkPipelineLayout()const
		{
			return layout();
		}

		constexpr const auto& setsLayouts()const
		{
			return _sets;
		}

		constexpr const auto& pushConstants()const
		{
			return _push_constants;
		}
	};

	class PipelineLayout : public InstanceHolder<PipelineLayoutInstance>
	{
	public:
		using ParentType = InstanceHolder<PipelineLayoutInstance>;
	protected:

		bool _is_dynamic = false;
		size_t _update_tick = 0;
		
		VkPipelineLayoutCreateFlags _flags = 0;
		std::vector<std::shared_ptr<DescriptorSetLayout>> _sets = {};
		std::vector<VkPushConstantRange> _push_constants = {};

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::vector<std::shared_ptr<DescriptorSetLayout>> sets = {};
			std::vector<VkPushConstantRange> push_constants = {};
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		PipelineLayout(CreateInfo const& ci);

		virtual ~PipelineLayout() override;

		void createInstance();

		bool updateResources(UpdateContext & ctx);

		constexpr const auto& setsLayouts()const
		{
			return _sets;
		}

		constexpr const auto& pushConstants()const
		{
			return _push_constants;
		}

		constexpr bool isDynamic() const
		{
			return _is_dynamic;
		}
	};
}