#include "Sampler.hpp"

namespace vkl
{
	Sampler::Sampler(VkApplication* app, VkSamplerCreateInfo const& ci) :
		VkObject(app)
	{
		assert(_app);
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
}