#include "Sampler.hpp"

namespace vkl
{
	Sampler::Sampler(VkApplication* app, VkSamplerCreateInfo const& ci) :
		VkObject(app)
	{
		assert(_app);
		createSampler(ci);
	}

	Sampler::Sampler(VkApplication* app, VkFilter filter, VkSamplerAddressMode address_mode, VkBool32 anisotropy, VkBool32 unormalize_coords, VkBorderColor border) :
		VkObject(app)
	{
		VkSamplerCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = filter,
			.minFilter = filter,
			.mipmapMode = mipMapModeFromFilter(filter),
			.addressModeU = address_mode,
			.addressModeV = address_mode,
			.addressModeW = address_mode,
			.mipLodBias = 0,
			.anisotropyEnable = anisotropy,
			.maxAnisotropy = 16, // TODO
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0,
			.maxLod = 1, // TODO
			.borderColor = border,
			.unnormalizedCoordinates = unormalize_coords,
		};

		createSampler(ci);
	}

	Sampler::~Sampler()
	{
		if (_sampler != VK_NULL_HANDLE)
		{
			destroySampler();
		}
	}

	void Sampler::createSampler(VkSamplerCreateInfo const& ci)
	{
		VK_CHECK(vkCreateSampler(_app->device(), &ci, nullptr, &_sampler), "Failed to create a sampler.");
	}

	void Sampler::destroySampler()
	{
		assert(_sampler != VK_NULL_HANDLE);
		vkDestroySampler(_app->device(), _sampler, nullptr);
		_sampler = VK_NULL_HANDLE;
	}

	Sampler Sampler::Nearest(VkApplication* app)
	{
		return Sampler(app, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_FALSE, VK_FALSE);
	}

	Sampler Sampler::Bilinear(VkApplication* app)
	{
		return Sampler(app, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_FALSE, VK_FALSE);
	}
}