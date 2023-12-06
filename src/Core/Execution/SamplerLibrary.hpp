#pragma once

#include <Core/VkObjects/Sampler.hpp>
#include <unordered_map>
#include <atomic>

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
		
		std::atomic<size_t> _sampler_count = 0;
		std::unordered_map<SamplerInfo, std::shared_ptr<Sampler>, Hasher> _map;
		std::unordered_map<std::string, std::shared_ptr<Sampler>> _named_samplers;

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

		void setNamedSampler(std::string const& name, std::shared_ptr<Sampler> const& s);
		
		void updateResources(UpdateContext & ctx);
	};
}