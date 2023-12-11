#include "SamplerLibrary.hpp"

namespace vkl
{
	bool SamplerLibrary::SamplerInfo::operator==(SamplerInfo const& o)const
	{
		return	(flags == o.flags) &&
				(filter == o.filter) &&
				(address_mode == o.address_mode) &&
				(max_anisotropy == o.max_anisotropy) && 
				(border_color == o.border_color) &&
				(unnormalized_coordinates == o.unnormalized_coordinates);
	}

	size_t SamplerLibrary::Hasher::operator()(SamplerInfo const& si) const
	{
		size_t res = 0;
		std::hash<uint32_t> h;
		std::hash<float> hf;
		std::hash<size_t> hs;
		res = hs(h(si.flags) ^ h(si.filter)) ^ hs(h(si.address_mode) ^ hf(si.max_anisotropy)) ^ hs(h(si.border_color) ^ h(si.unnormalized_coordinates));
		return res;
	}

	SamplerLibrary::SamplerLibrary(CreateInfo const& ci):
		VkObject(ci.app, ci.name)
	{}

	std::shared_ptr<Sampler> SamplerLibrary::getSampler(SamplerInfo const& si)
	{
		std::unique_lock lock(_mutex);
		if (!_map.contains(si))
		{
			const size_t i = _sampler_count.fetch_add(1);
			std::shared_ptr<Sampler> res = std::make_shared<Sampler>(Sampler::CI{
				.app = application(),
				.name = name() + ".sampler#"s + std::to_string(i),
				.flags = si.flags,
				.filter = si.filter,
				.address_mode = si.address_mode,
				.max_anisotropy = si.max_anisotropy,
				.border_color = si.border_color,
				.unnormalized_coordinates = si.unnormalized_coordinates,
			});
			_map[si] = res;
			return res;
		}
		return _map[si];
	}

	std::shared_ptr<Sampler> SamplerLibrary::getNamedSampler(std::string const& name) const
	{
		std::unique_lock lock(_mutex);
		assert(_named_samplers.contains(name));
		return _named_samplers.at(name);
	}

	void SamplerLibrary::setNamedSampler(std::string const& name, std::shared_ptr<Sampler> const& s)
	{
		std::unique_lock lock(_mutex);
		_named_samplers[name] = s;
	}

	void SamplerLibrary::updateResources(UpdateContext& ctx)
	{
		std::unique_lock lock(_mutex);
		for (auto& [si, s] : _map)
		{
			s->updateResources(ctx);
		}
		for (auto& [n, s] : _named_samplers)
		{
			s->updateResources(ctx);
		}
	}
}