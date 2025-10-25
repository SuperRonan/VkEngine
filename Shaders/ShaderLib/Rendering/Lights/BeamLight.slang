#pragma once

#include "Light.slang"

struct BeamLight<int Mode> : ILight<Mode>
{
	typealias Sample = Light::Sample;
	typealias SpectruM = Spectrumf<Mode>;

	vec3 position;
	uint flags;

	vec3 emission;
	
	vec3 direction;
	float aspect;

	vec3 up;
	float radius;


	__init(const in StorageLight l)
	{
		position = l.position;
		flags = l.flags;
		emission = l.emission;
		direction = l.direction;
		up = l.getUp();
		radius = l.getBeamRadius();
		aspect = l.getAspect();
	}

	uint getFlags()
	{
		return flags;
	}

	LightType getType()
	{
		return LightType::Beam;
	}

	Matrix3x4f getViewToWorld()
	{
		return InverseLookAtDirAssumeOrtho(position, direction, up);
	}

	Matrix3f getBasis()
	{
		const vec3 right = Cross(direction, up);
		return MakeFromRows(right, up, direction);
	}

	Spectrumf<Mode> getSpectralEmission()
	{
		return Light::GetSpectralEmission<Mode>(emission, flags);
	}

	vec3 getPositionCP(vec3 position)
	{
		vec3 d = position - this.position;
		vec3 res = getBasis() * d;
		if(radius != 0)
		{
			res.xy /= radius;
			if(aspect != 0)
			{
				res.x /= aspect;
			}
		}
		return res;
	}

	float evaluateAttenuation(float2 cp)
	{
		return Light::EvaluateAttenuation(flags, cp);
	}

	Light::ConnectionSample<Mode> sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position)
	{
		Light::ConnectionSample<Mode> res = {};
		if(radius != 0.0f && aspect != 0.0f)
		{
			const vec3 cp = getPositionCP(shading_position);
			if(all(cp.xy <= 1) && all(cp.xy >= -1) && cp.z > 0)
			{
				res.direction = -direction;
				res.pdf = 1;
				res.intensity = getSpectralEmission() * evaluateAttenuation(cp.xy);
				res.flags = Sample::Flags::DeltaDirection;
				res.distance = cp.z;
				res.extra = cp.xy;
			}
			else
			{
				res.pdf = 0;
			}
		}
		else
		{
			res.pdf = 0;
		}
		return res;
	}

	float pdfLi(vec3 reference_position, vec3 direction)
	{
		// No need to check if in the view
		return 1;
	}

	Light::EmissionSample<Mode> sampleLe<RNG : IRNG>(inout RNG rng)
	{
		Light::EmissionSample<Mode> res;
		if(radius == 0) // Sample a starting point
		{
			res.position_pdf = 1;
			res.pdf = 1;
			res.ray = Ray3f(position, direction);
			res.intensity = getSpectralEmission();
			res.flags = Sample::Flags::DeltaDirection | Sample::Flags::DeltaPosition;
		}
		else if(aspect == 0) // Sample a point on a segment
		{
			float x = rng.generate<float>(-1, 2) * radius;
			res.ray = Ray3f(this.position + getBasis().t * vec3(0, x, 0), direction);
			res.pdf = 1;
			res.position_pdf = 0.5 * rcp(radius);
			res.intensity = getSpectralEmission();
			res.flags = Sample::Flags::DeltaDirection | Sample::Flags::DeltaPosition;
		}
		else // Sample a point on a surface
		{
			vec2 xi = rng.generate<float, 2>();
			const uint attenuation_flags = flags & LIGHT_FLAG_ATTENUATION_MASK;
			::Sample<vec2> sample_cp;
			if((attenuation_flags) != 0)
			{
				sample_cp = SampleUniformOnDisk(xi);
			}
			else
			{
				sample_cp.value = UVToClipSpace(xi);
				sample_cp.pdf = 0.25;
			}
			res.ray = Ray3f(position + getBasis().t * vec3(sample_cp.value * radius * vec2(aspect != 0 ? aspect : 1, 1), 0), direction);
			res.pdf = 1;
			res.position_pdf = sample_cp.pdf * rcp(sqr(radius) * (aspect != 0 ? aspect : 1));
			res.intensity = getSpectralEmission() * evaluateAttenuation(sample_cp.value);
			res.flags = Sample::Flags::DeltaDirection;
		}
		res.range.resetRange();
		return res;
	}

	RayPdf pdfLe(Ray3f ray)
	{
		RayPdf res = {};
		if(true || all(ray.direction == direction) && radius != 0 && aspect != 0)
		{
			// The pdf outside of the sampling region should be zero,
			// But since the emission is zero too outside, it does not matter (with non optimal MIS)
			
			// let cp = getPositionCP(ray.origin);
			// if(all(cp.xy <= 1) && all(cp.xy >= -1) && cp.z > 0)
			{
				const uint attenuation_flags = flags & LIGHT_FLAG_ATTENUATION_MASK;
				float area_pdf = 0;
				if(attenuation_flags != 0)
				{
					// if(Length2(cp.xy) <= 1)
					{
						area_pdf = oo_PI;
					}
				}
				else
				{
					area_pdf = 0.25;
				}
				res.position_pdf = area_pdf * rcp(sqr(radius) * (aspect));
			}
			res.direction_pdf = 1;
		}
		return res;
	}
}