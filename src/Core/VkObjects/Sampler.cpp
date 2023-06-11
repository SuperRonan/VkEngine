#include "Sampler.hpp"

namespace vkl
{
	SamplerInstance::SamplerInstance(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_ci(ci.vk_ci)
	{
		VK_CHECK(vkCreateSampler(device(), &_ci, nullptr, &_sampler), "Failed to create a sampler.");
	}

	SamplerInstance::~SamplerInstance()
	{
		assert(_sampler != VK_NULL_HANDLE);
		vkDestroySampler(device(), _sampler, nullptr);
		_sampler = VK_NULL_HANDLE;
	}

	Sampler::Sampler(CreateInfo const& ci) :
		InstanceHolder<SamplerInstance>(ci.app, ci.name)
	{
		_vk_ci = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = ci.filter,
			.minFilter = ci.filter,
			.mipmapMode = mipMapModeFromFilter(ci.filter),
			.addressModeU = ci.address_mode,
			.addressModeV = ci.address_mode,
			.addressModeW = ci.address_mode,
			.mipLodBias = 0,
			.anisotropyEnable = ci.max_anisotropy.has_value(),
			.maxAnisotropy = ci.max_anisotropy.value_or(0.0f), 
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0,
			.maxLod = 1, // TODO
			.borderColor = ci.border_color,
			.unnormalizedCoordinates = ci.unnormalized_coordinates,
		};

		if (ci.create_on_construct)
		{
			createInstance();
		}
	}

	Sampler::~Sampler()
	{
		destroyInstance();
	}

	void Sampler::createInstance()
	{
		destroyInstance();
		
		_inst = std::make_shared<SamplerInstance>(SamplerInstance::CI{
			.app = application(),
			.name = name(),
			.vk_ci = _vk_ci,
		});
	}

	void Sampler::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	bool Sampler::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		if (!_inst)
		{
			createInstance();
			res = true;
		}

		return res;
	}

	std::shared_ptr<Sampler> Sampler::MakeNearest(VkApplication* app)
	{
		return std::make_shared<Sampler>(Sampler::CI{
			.app = app,
			.name = "NearestSampler",
			.filter = VK_FILTER_NEAREST,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		});
	}

	std::shared_ptr<Sampler> Sampler::MakeBilinear(VkApplication* app)
	{
		return std::make_shared<Sampler>(Sampler::CI{
			.app = app,
			.name = "LinearSampler",
			.filter = VK_FILTER_LINEAR,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		});
	}
}