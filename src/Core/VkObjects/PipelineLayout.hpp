#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/VkObjects/DescriptorSetLayout.hpp>

namespace vkl
{
	class PipelineLayout : public VkObject
	{
	protected:

		VkPipelineLayoutCreateFlags _flags = 0;
		std::vector<std::shared_ptr<DescriptorSetLayout>> _sets = {};
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
			std::vector<std::shared_ptr<DescriptorSetLayout>> sets = {};
			std::vector<VkPushConstantRange> push_constants = {};
		};
		using CI = CreateInfo;

		PipelineLayout(CreateInfo const& ci);

		virtual ~PipelineLayout() override;


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
}