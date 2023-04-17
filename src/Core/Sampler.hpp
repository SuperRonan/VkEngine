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

		constexpr static VkBorderColor defaultBorderColor()
		{
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		}

		constexpr static VkSamplerMipmapMode mipMapModeFromFilter(VkFilter filter)
		{
			if (filter == VK_FILTER_NEAREST)	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			else //if (filter == VK_FILTER_LINEAR)
				return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}

		constexpr Sampler()noexcept = default;

		constexpr Sampler(VkApplication* app, VkSampler handle) :
			VkObject(app),
			_sampler(handle)
		{}

		Sampler(VkApplication* app, VkSamplerCreateInfo const& ci);

		Sampler(VkApplication* app, VkFilter filter, VkSamplerAddressMode address_mode, VkBool32 anisotropy, VkBool32 unormalize_coords, VkBorderColor border = defaultBorderColor());

		virtual ~Sampler() override;

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

		static Sampler Nearest(VkApplication * app = nullptr);

		static Sampler Bilinear(VkApplication* app = nullptr);

	};
}