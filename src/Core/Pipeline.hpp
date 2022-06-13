#pragma once

#include "VkApplication.hpp"
#include "Program.hpp"

namespace vkl
{
	class Pipeline : public VkObject
	{
	protected:

		VkPipeline _handle = VK_NULL_HANDLE;
		VkPipelineBindPoint _binding;
		std::shared_ptr<Program> _program;

	public:

		constexpr Pipeline() noexcept = default;

		constexpr Pipeline(VkApplication* app, VkPipeline handle, VkPipelineBindPoint type) noexcept:
			VkObject(app),
			_handle(handle),
			_binding(type)
		{}

		Pipeline(Pipeline const&) = delete;

		constexpr Pipeline(Pipeline&& other) noexcept :
			VkObject(std::move(other)),
			_handle(other._handle),
			_binding(other._binding)
		{
			other._handle = VK_NULL_HANDLE;
		}

		Pipeline(VkApplication* app, VkGraphicsPipelineCreateInfo const& ci);

		Pipeline(VkApplication* app, VkComputePipelineCreateInfo const& ci);

		Pipeline& operator=(Pipeline const&) = delete;

		constexpr Pipeline& operator=(Pipeline&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_handle, other._handle);
			std::swap(_binding, other._binding);
			return *this;
		}

		virtual ~Pipeline();

		void createPipeline(VkGraphicsPipelineCreateInfo const& ci);

		void createPipeline(VkComputePipelineCreateInfo const& ci);

		void destroyPipeline();

		constexpr VkPipeline pipeline()const noexcept
		{
			return _handle;
		}

		constexpr auto handle()const noexcept
		{
			return pipeline();
		}

		constexpr VkPipelineBindPoint binding()const noexcept
		{
			return _binding;
		}

		constexpr operator VkPipeline()const
		{
			return _handle;
		}
	};
}