#pragma once

#include "Light.slang"

// Models a uniform sky with a solar disk
struct SkyLight<int Mode> : ILight<Mode>
{
	typealias Sample = Light::Sample;
	typealias SpectruM = Spectrumf<Mode>;
	
	SpectruM uniform_emission;
	uint flags = 0;
	SpectruM solar_disk_emission;
	float solar_disk_angle;
	vec3 solar_disk_direction;
	float solar_disk_cosine;

	__init(const in StorageLight l) {}

	uint getFlags()
	{
		return flags;
	}

	LightType getType()
	{
		return LightType::EnvMap;
	}
	
	SpectruM Le(vec3 direction)
	{
		SpectruM res = uniform_emission;
		float ct = dot(direction, solar_disk_direction);
		if(ct > solar_disk_cosine)
		{
			res += solar_disk_emission;
		}
		return res;
	}

	float getPMFUniform(const in SpectruM filter)
	{
		float uni_weight = ConeSolidAngle(-1) * (uniform_emission * filter).fastEnergy();
		float solar_weight = ConeSolidAngle(solar_disk_cosine) * (solar_disk_emission * filter).fastEnergy();
		float pmf_uni = uni_weight / (uni_weight + solar_weight);
		return pmf_uni;
	}

	struct DirectionSample
	{
		SpectruM emission;
		vec3 direction; // to env map
		float pdf;
	}

	// Common for sampleLe and sampleLi
	DirectionSample sampleDirection<RNG : IRNG>(inout RNG rng, const in SpectruM filter)
	{
		DirectionSample res;
		// This does not take into account the bsdf shape
		float sampling_cosine_angle = -1;
		float choice_xi = rng.generate<float>();
		float pmf_uniform = getPMFUniform(filter);
		bool sample_uniform = choice_xi <= pmf_uniform;
		if(sample_uniform)
		{
			sampling_cosine_angle = -1;
		}
		else
		{
			sampling_cosine_angle = solar_disk_cosine;
		}
		::Sample<vec3> dir_sample = SampleUniformDirection(rng.generate<float, 2>(), sampling_cosine_angle);
		float uni_pdf = rcp(ConeSolidAngle(-1));
		float solar_pdf = 0;
		res.emission = uniform_emission;
		if(sample_uniform)
		{
			if(dot(dir_sample.value, solar_disk_direction) > solar_disk_cosine)
			{
				solar_pdf = rcp(ConeSolidAngle(solar_disk_cosine));
				res.emission += solar_disk_emission;
			}
		}
		else
		{
			let B = BasisFromDir(solar_disk_direction);
			dir_sample.value = B * dir_sample.value;
			solar_pdf = dir_sample.pdf;
			res.emission += solar_disk_emission;
		}
		res.direction = dir_sample.value;
		res.pdf = lerp(solar_pdf, uni_pdf, pmf_uniform);
		return res;
	}

	float getDirectionPDF(vec3 direction, SpectruM filter)
	{
		float res = 0;
		float pmf_uniform = getPMFUniform(filter);
		float pdf_uni = rcp(ConeSolidAngle(-1));
		float pdf_solar = 0;
		if(dot(direction, solar_disk_direction) > solar_disk_cosine)
		{
			pdf_solar = rcp(ConeSolidAngle(solar_disk_cosine));
		}
		res = lerp(pdf_solar, pdf_uni, pmf_uniform);
		return res;
	}

	// Directions can be zero (means no direction)
	Light::ConnectionSample<Mode> sampleLi<RNG : IRNG>(inout RNG rng, vec3 reference_position, SpectruM filter)
	{
		Light::ConnectionSample<Mode> res;
		let dir_sample = sampleDirection(rng, filter);
		res.direction = dir_sample.direction;
		res.intensity = dir_sample.emission;
		res.pdf = dir_sample.pdf;
		res.distance = std::numeric_limits<float>::infinity();
		res.flags = Sample::Flags::AtInfinity;
		return res;
	}

	Light::ConnectionSample<Mode> sampleLi<RNG : IRNG>(inout RNG rng, vec3 reference_position)
	{
		return sampleLi(rng, reference_position, SpectruM::One());
	}
	
	float pdfLi(vec3 reference_position, vec3 direction, const in SpectruM filter)
	{
		return getDirectionPDF(direction, filter);
	}

	float pdfLi(vec3 reference_position, vec3 direction)
	{
		return pdfLi(reference_position, direction, SpectruM::One());
	}
	
	Light::EmissionSample<Mode> sampleLe<RNG : IRNG>(inout RNG rng)
	{
		Light::EmissionSample<Mode> res;
		let dir_sample = sampleDirection(rng, SpectruM::One());
		res.ray.direction = -dir_sample.direction;
		res.intensity = dir_sample.emission;
		res.position_pdf = 1;
		res.pdf = dir_sample.pdf;
		res.flags = Sample::Flags::AtInfinity;
		res.range.resetRange();
		return res;
	}

	RayPdf pdfLe(Ray3f ray)
	{
		RayPdf res = {};
		res.direction_pdf = getDirectionPDF(-ray.direction, SpectruM::One());
		res.position_pdf = 1;
		return res;
	}
};