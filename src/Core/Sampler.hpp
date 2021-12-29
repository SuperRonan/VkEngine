#pragma once

#include "VkApplication.hpp"
#include <cassert>
#include <utility>

namespace vkl
{
	class Sampler : public VkObject
	{
	protected:

		VkSampler _sampler = VK_NULL_HANDLE;

	public:

		constexpr Sampler()noexcept = default;

		constexpr Sampler(VkApplication* app, VkSampler handle) :
			VkObject(app),
			_sampler(handle)
		{}

		Sampler(VkApplication* app, VkSamplerCreateInfo const& ci);

		Sampler(Sampler const&) = delete;

		constexpr Sampler(Sampler&& other) noexcept :
			VkObject(other),
			_sampler(other._sampler)
		{
			other._sampler = VK_NULL_HANDLE;
		}

		Sampler& operator=(Sampler const& other) = delete;

		constexpr Sampler& operator=(Sampler&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_sampler, other._sampler);
			return *this;
		}

		~Sampler();

		void createSampler(VkSamplerCreateInfo const& ci);

		void destroySampler();

		constexpr VkSampler handle()const
		{
			return sampler();
		}

		constexpr VkSampler sampler()const
		{
			return _sampler;
		}

		constexpr operator VkSampler()const
		{
			return sampler();
		}

	};
}