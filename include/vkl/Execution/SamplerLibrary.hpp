#pragma once

#include <vkl/VkObjects/Sampler.hpp>

#include <unordered_map>
#include <atomic>
#include <shared_mutex>

namespace vkl
{
	class SamplerLibrary : public VkObject
	{
	public:
		struct SamplerInfo
		{
			VkSamplerCreateFlags flags = 0;
			VkFilter filter = VK_FILTER_NEAREST;
			VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			float max_anisotropy = 1.0;
			VkBorderColor border_color = {};
			VkBool32 unnormalized_coordinates = false;

			bool operator==(SamplerInfo const& o) const;
		};
	protected:

		struct Hasher
		{
			size_t operator()(SamplerInfo const& si) const;
		};

		mutable std::shared_mutex _mutex;
		
		std::atomic<size_t> _sampler_count = 0;
		std::unordered_map<SamplerInfo, std::shared_ptr<Sampler>, Hasher> _map;
		std::unordered_map<std::string, std::shared_ptr<Sampler>> _named_samplers;
		std::shared_ptr<Sampler> _default_sampler;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		SamplerLibrary(CreateInfo const& ci);

		std::shared_ptr<Sampler> getSampler(SamplerInfo const& si);

		std::shared_ptr<Sampler> getNamedSampler(std::string const& name) const;

		std::shared_ptr<Sampler> const& getDefaultSampler()const
		{
			return _default_sampler;
		}

		void setNamedSampler(std::string const& name, std::shared_ptr<Sampler> const& s);
		
		void updateResources(UpdateContext & ctx);
	};
}