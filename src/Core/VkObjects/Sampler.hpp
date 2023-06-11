#pragma once

#include "AbstractInstance.hpp"
#include <cassert>
#include <utility>
#include <Core/Execution/UpdateContext.hpp>

namespace vkl
{
	class SamplerInstance: public VkObject
	{
	protected:

		VkSampler _sampler = VK_NULL_HANDLE;
		VkSamplerCreateInfo _ci = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkSamplerCreateInfo vk_ci = {};
		};
		using CI = CreateInfo;

		SamplerInstance(CreateInfo const& ci);

		virtual ~SamplerInstance() override;

		constexpr const VkSamplerCreateInfo& createInfo()const
		{
			return _ci;
		}

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

	class Sampler : public InstanceHolder<SamplerInstance>
	{
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

	protected:

		void createInstance();

		void destroyInstance();

		VkSamplerCreateInfo _vk_ci = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkSamplerCreateFlags flags = 0;
			VkFilter filter = VK_FILTER_NEAREST;
			VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			std::optional<float> max_anisotropy = {};
			VkBorderColor border_color = {};
			VkBool32 unnormalized_coordinates = false;
			bool create_on_construct = false;
		};
		using CI = CreateInfo;

		Sampler(CreateInfo const& ci);

		virtual ~Sampler() override;

		bool updateResources(UpdateContext & ctx);

		static std::shared_ptr<Sampler> MakeNearest(VkApplication * app = nullptr);

		static std::shared_ptr<Sampler> MakeBilinear(VkApplication* app = nullptr);

	};
}