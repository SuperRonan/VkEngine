#pragma once

#include "Light.slang"

struct PointLight<int Mode> : ILight<Mode>
{
	typealias SpectruM = Spectrumf<Mode>;
	typealias Sample = Light::Sample;

	vec3 position;
	uint flags;

	vec3 emission;
	uint shadow_bias_data;

	float z_near;
	uint shadow_texture_index;

	__init(const in StorageLight l)
	{
		position = l.position;
		flags = l.flags;
		emission = l.emission;
		shadow_bias_data = l.shadow_bias_data;
		z_near = l.z_near;
		shadow_texture_index = l.textures.x;
	}

	uint getFlags()
	{
		return flags;
	}

	LightType getType()
	{
		return LightType::Point;
	}

	SpectruM getSpectralEmission()
	{
		return Light::GetSpectralEmission<Mode>(emission, flags);
	}

	Light::ConnectionSample<Mode> sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position)
	{
		Light::ConnectionSample<Mode> res;
		const vec3 d = position - shading_position;
		const float d2 = Length2(d);
		res.direction = Normalize(d);
		res.intensity = getSpectralEmission();
		res.pdf = d2;
		res.flags = Sample::Flags::DeltaPosition;
		res.distance = Length(d);
		return res;
	}

	float pdfLi(vec3 reference_position, vec3 direction)
	{
		return Length2(direction);
	}

	Light::EmissionSample<Mode> sampleLe<RNG : IRNG>(inout RNG rng)
	{
		Light::EmissionSample<Mode> res;
		res.ray.origin = position;
		let dir_sample = SampleUniformDirection(rng.generate<float, 2>());
		res.ray.direction = dir_sample.value;
		res.range = {};
		res.pdf = dir_sample.pdf;
		res.position_pdf = 1;
		res.intensity = getSpectralEmission();
		res.flags = Sample::Flags::DeltaPosition;
		return res;
	}

	RayPdf pdfLe(Ray3f ray)
	{
		RayPdf res = {};
		if(true || all(ray.origin == position))
		{
			res.position_pdf = 1;
			res.direction_pdf = 0.25 * oo_PI;
		}
		return res;
	}

	mat4 getWorldToProj(uint face)
	{
		return GetCubeMapFaceWorldToProj(face, position, z_near);
	}
};