#pragma once

#include "Light.slang"

struct SpotLight<int Mode> : ILight<Mode>
{
	typealias Sample = Light::Sample;
	typealias SpectruM = Spectrumf<Mode>;

	vec3 position;
	uint flags;

	vec3 emission;
	uint shadow_bias_data;

	vec3 direction;
	float z_near;
	
	vec3 up;
	float tan_half_fov;
	
	float aspect;
	uint shadow_texture_index;

	__init(const in StorageLight l)
	{
		position = l.position;
		flags = l.flags;

		emission = l.emission;
		shadow_bias_data = l.shadow_bias_data;
		
		direction = l.direction;
		z_near = l.z_near;
		
		up = l.getUp();
		tan_half_fov = l.getTanHalfFOV();
		
		aspect = l.getAspect();
		shadow_texture_index = l.textures.x;
	}

	uint getFlags()
	{
		return flags;
	}

	LightType getType()
	{
		return LightType::Spot;
	}

	Matrix4f getWorldToProj()
	{
		return InfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near, true) * ResizeMatrix<4, 4>(LookAtDirAssumeOrtho(position, direction, up));
	}

	Matrix3x4f getViewToWorld()
	{
		return InverseLookAtDirAssumeOrtho(position, direction, up);
	}

	Matrix3f getBasis()
	{
		const vec3 r = Cross(direction, up);
		return MakeFromRows(r, up, direction);
	}

	Matrix3x4f getProjToWorld(float z_near)
	{
		return getViewToWorld() * InverseInfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near, true);
	}

	Matrix3x4f getProjToWorld()
	{
		return getProjToWorld(z_near);
	}

	Spectrumf<Mode> getSpectralEmission()
	{
		return Light::GetSpectralEmission<Mode>(emission, flags);
	}

	

	float getInvArea()
	{
		return rcp(aspect);
	}

	// direction going out from the camera, and is normalized
	Optional<vec2> getDirectionCP(vec3 direction)
	{
		Optional<vec2> res = none;
		vec3 dir_c = Transpose(getBasis()) * direction;
		const float z = dir_c.z;
		if(z > 0)
		{
			vec2 cp = dir_c.xy / z;
			cp.x *= rcp(this.aspect);
			cp = cp * rcp(tan_half_fov);
			if(all(cp <= 1..xx) && all(cp >= -1..xx))
			{
				res = cp;
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
		Light::ConnectionSample<Mode> res;
		const vec3 d = position - shading_position;
		const float dist2 = Length2(d);
		res.direction = Normalize(d);
		res.distance = Length(d);
		res.pdf = dist2;
		const vec4 proj_shading_position_h = getWorldToProj() * Homogeneous(shading_position);
		const vec3 proj_shading_position = proj_shading_position_h.xyz / proj_shading_position_h.w;
		const float ct = -Dot(res.direction, this.direction);
		const bool in_frustum = ct > 0 && all(proj_shading_position.xy <= 1..xx) && all(proj_shading_position.xy >= -1..xx);
		if(in_frustum)
		{
			res.extra.xy = proj_shading_position.xy;
			
			const float ct3 = ct * ct * ct;
			float density_conversion = (rcp(ct3 * sqr(this.tan_half_fov)) * getInvArea());
			res.intensity = getSpectralEmission() * (density_conversion * evaluateAttenuation(res.extra.xy));
		}
		else
		{
			res.intensity = Spectrumf<Mode>::Zero();
		}
		res.flags = Light::Sample::Flags::DeltaPosition;
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
		// sample the cp that considers the attenuation
		vec2 xi = rng.generate<float, 2>();
		::Sample<vec2> sample_cp;
		const uint attenuation_flags = flags & LIGHT_FLAG_ATTENUATION_MASK;
		if((attenuation_flags) != 0)
		{
			sample_cp = SampleUniformOnDisk(xi);
		}
		else
		{
			sample_cp.value = UVToClipSpace(xi);
			sample_cp.pdf = 0.25;
		}
		// Explicit z_near at 1
		// Here the z_near does not mathematically matter,
		// but with a "usual" value like 1e-3, it makes a loss of precision in the matrix product
		// which induces some aliasing artifacts in the sampling
		res.ray.direction = Normalize(getProjToWorld(1) * vec4(sample_cp.value, 1, 1));
		res.range = {};
		const float ct = Dot(res.ray.direction, this.direction);
		const float ct3 = ct * ct * ct;
		const float density_conversion = (rcp(ct3 * sqr(this.tan_half_fov)) * getInvArea());
		res.pdf = sample_cp.pdf * density_conversion;
		res.position_pdf = 1;
		res.intensity = getSpectralEmission() * (density_conversion * evaluateAttenuation(sample_cp.value));
		res.flags = Sample::Flags::DeltaPosition;
		return res;
	}

	RayPdf pdfLe(Ray3f ray)
	{
		RayPdf res = {};
		if(true || all(ray.origin == position))
		{
			let cp = getDirectionCP(ray.direction);
			if(cp.hasValue)
			{
				vec3 dir_c = Transpose(getBasis()) * ray.direction;
				const float ct = dir_c.z;
				const float ct3 = ct * ct * ct;
				float area_pdf = 0;
				const uint attenuation_flags = flags & LIGHT_FLAG_ATTENUATION_MASK;
				if(attenuation_flags != 0)
				{
					const float l2 = Length2(cp.value);
					if(l2 <= 1)
					{
						area_pdf = oo_PI;
					}
				}
				else
				{
					area_pdf = 0.25;
				}
				res.direction_pdf = area_pdf * rcp(ct3 * sqr(this.tan_half_fov)) * getInvArea();
			}
			res.position_pdf = 1;
		}
		return res;
	}
};