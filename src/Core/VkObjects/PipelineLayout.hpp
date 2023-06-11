#pragma once

#include <Core/App/VkApplication.hpp>

namespace vkl
{
	class PipelineLayout : public VkObject
	{
	protected:

		VkPipelineLayout _layout = VK_NULL_HANDLE;

	public:

		constexpr PipelineLayout(VkApplication * app = nullptr, VkPipelineLayout layout = VK_NULL_HANDLE):
			VkObject(app),
			_layout(layout)
		{}

		PipelineLayout(VkApplication* app, VkPipelineLayoutCreateInfo const& ci);

		~PipelineLayout();

		void create(VkPipelineLayoutCreateInfo const& ci);

		void destroy();

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
	};
}