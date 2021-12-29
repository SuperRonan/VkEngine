#pragma once

#include "VkApplication.hpp"

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

		constexpr PipelineLayout(PipelineLayout const&) noexcept = delete;

		constexpr PipelineLayout(PipelineLayout&& other) noexcept :
			VkObject(std::move(other)),
			_layout(other._layout)
		{
			other._layout = VK_NULL_HANDLE;
		}

		constexpr PipelineLayout& operator=(PipelineLayout const&) noexcept = delete;

		constexpr PipelineLayout& operator=(PipelineLayout&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_layout, other._layout);
			return *this;
		}

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