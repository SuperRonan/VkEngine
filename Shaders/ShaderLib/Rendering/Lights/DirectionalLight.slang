#pragma once

#include "Light.slang"

struct DirectionalLight<int Mode> : ILight<Mode>
{
	typealias Sample = Light::Sample;
	typealias SpectruM = Spectrumf<Mode>;

	vec3 direction;
	uint flags;

	vec3 emission;
	uint shadow_bias_data;

	vec3 up;
	float z_near;
	uint shadow_texture_index;

	__init(const in StorageLight l)
	{
		direction = l.direction;
		flags = l.flags;
		
		emission = l.emission;
		shadow_bias_data = l.shadow_bias_data;
		
		up = l.getUp();
		z_near = l.z_near;
		shadow_texture_index = l.textures.x;
	}

	uint getFlags()
	{
		return flags;
	}

	LightType getType()
	{
		return LightType::Directional;
	}

	Spectrumf<Mode> getSpectralEmission<int Mode>()
	{
		return Spectrumf<Mode>::MakeFromRGB(emission, RGBSourceType::Illuminant);
	}

	Light::ConnectionSample<Mode> sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position)
	{
		Light::ConnectionSample<Mode> res;
		res.direction = direction;
		res.intensity = getSpectralEmission<Mode>();
		res.pdf = 1;
		res.flags = Light::Sample::Flags::AtInfinity | Light::Sample::Flags::DeltaDirection;
		res.distance = NumericLimits<float>::infinity();
		return res;
	}

	float pdfLi(vec3 reference_position, vec3 direction)
	{
		return 1;
	}

	Light::EmissionSample<Mode> sampleLe<RNG : IRNG>(inout RNG rng)
	{
		Light::EmissionSample<Mode> res;
		res.ray.direction = -direction;
		res.intensity = getSpectralEmission<Mode>();
		res.position_pdf = 1;
		res.pdf = 1;
		res.flags = Light::Sample::Flags::AtInfinity | Light::Sample::Flags::DeltaDirection;
		res.range.resetRange();
		return res;
	}

	RayPdf pdfLe(Ray3f ray)
	{
		RayPdf res = {};
		res.direction_pdf = 0;
		res.position_pdf = 0;
		return res;
	}
};