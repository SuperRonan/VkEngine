#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/View.slang>
#include <ShaderLib/Maths/Sampling.slang>
#include <ShaderLib/Random.slang>

#include <ShaderLib/Maths/NumericLimits.slang>

#include <ShaderLib/Rendering/Ray.slang>

#include "Definitions.h"

enum LightType
{
	Point = LIGHT_TYPE_POINT,
	Directional = LIGHT_TYPE_DIRECTIONAL,
	Spot = LIGHT_TYPE_SPOT,
};

struct StorageLight
{
	vec3 position;
	uint flags; // 4
	
	vec3 emission;
	// int or float
	uint shadow_bias_data; // 8
	
	uvec4 textures; // 12

	vec3 direction;
	float z_near; // 16

	u32 extra_params[16]; // 32

	LightType getType()
	{
		return LightType(flags & LIGHT_TYPE_MASK);
	}

	vec3 getUp()
	{
		vec3 res;
		const uint b = 0;
		for(uint i = 0; i < 3; ++i)
		{
			res[i] = bit_cast<float>(extra_params[b + i]);
		}
		return res;
	}

	float getTanHalfFOV()
	{
		return bit_cast<float>(extra_params[3]);
	}

	float getAspect()
	{
		return bit_cast<float>(extra_params[4]);
	}
};

struct Light
{
	struct Sample
	{
		[Flags]
		enum Flags
		{
			None 			= 0x0,
			DeltaPosition 	= 0x1,
			DeltaDirection 	= 0x2,
			AtInfinity 		= 0x4,
		};

		vec3 intensity;
		PackedEnumIndex<Flags, 8, u32> flags_index;
		float pdf;

		property Flags flags
		{
			get {return flags_index.enum;}
			set {flags_index.enum = newValue;}
		}

		property u32 index
		{
			get {return flags_index.index;}
			set {flags_index.index = newValue;}
		}
	};

	struct ConnectionSample : Sample
	{
		vec3 direction;

		float distance;
		// Optional additional fields
		vec2 extra;
	};

	struct EmissionSample : Sample
	{
		Ray3f ray;
		RayRangef range;
		float position_pdf;
	};
};

interface ILight
{
	__init(const in StorageLight l);

	uint getFlags();

	LightType getType();

	// Directions can be zero (means no direction)
	Light::ConnectionSample sampleLi<RNG : IRNG>(inout RNG rng, vec3 reference_position);
	
	// Direction not normalized
	// Assume reference_position + direction is on the light
	float pdfLi(vec3 reference_position, vec3 direction);
	
	Light::EmissionSample sampleLe<RNG : IRNG>(inout RNG rng);

	RayPdf pdfLe(Ray3f ray);
}; 


struct PointLight : Light, ILight
{
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

	Light::ConnectionSample sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position)
	{
		Light::ConnectionSample res;
		const vec3 d = position - shading_position;
		const float d2 = Length2(d);
		res.direction = Normalize(d);
		res.intensity = emission;
		res.pdf = d2;
		res.flags = Sample::Flags::DeltaPosition;
		res.distance = Length(d);
		return res;
	}

	float pdfLi(vec3 reference_position, vec3 direction)
	{
		return Length2(direction);
	}

	Light::EmissionSample sampleLe<RNG : IRNG>(inout RNG rng)
	{
		Light::EmissionSample res;
		res.ray.origin = position;
		let dir_sample = SampleUniformDirection(rng.generate<float, 2>());
		res.ray.direction = dir_sample.value;
		res.range = {};
		res.pdf = dir_sample.pdf;
		res.position_pdf = 1;
		res.intensity = emission;
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
}

struct DirectionalLight :  Light, ILight
{
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

	Light::ConnectionSample sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position)
	{
		Light::ConnectionSample res;
		res.direction = direction;
		res.intensity = emission;
		res.pdf = 1;
		res.flags = Light::Sample::Flags::AtInfinity || Light::Sample::Flags::DeltaDirection;
		res.distance = NumericLimits<float>::infinity();
		return res;
	}

	float pdfLi(vec3 reference_position, vec3 direction)
	{
		return 1;
	}

	Light::EmissionSample sampleLe<RNG : IRNG>(inout RNG rng)
	{
		// TODO
		Light::EmissionSample res;
		res.pdf = 0;
		return res;
	}

	RayPdf pdfLe(Ray3f ray)
	{
		// TODO
		RayPdf res = {};
		return res;
	}
}

struct SpotLight : Light, ILight
{
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
		return InfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near) * ResizeMatrix<4, 4>(LookAtDirAssumeOrtho(position, direction, up));
	}

	Matrix3x4f getViewToProj()
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
		return getViewToProj() * InverseInfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near);
	}

	Matrix3x4f getProjToWorld()
	{
		return getProjToWorld(z_near);
	}


	float evaluateAttenuation(vec2 cp)
	{
		float res = 1;
		const uint attenuation_flags = flags & SPOT_LIGHT_FLAG_ATTENUATION_MASK;
		if((attenuation_flags) != 0)
		{
			float dist_to_center = 0;
			if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_LINEAR)
			{
				dist_to_center = length(cp);
			}
			else if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_QUADRATIC)
			{
				dist_to_center = Length2(cp);
			}
			else if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_ROOT)
			{
				dist_to_center = sqrt(length(cp));
			}
			
			res = max(1.0 - dist_to_center, 0);
		}
		return res;
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

	Light::ConnectionSample sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position)
	{
		Light::ConnectionSample res;
		const vec4 proj_shading_position_h = getWorldToProj() * Homogeneous(shading_position);
		const vec3 proj_shading_position = proj_shading_position_h.xyz / proj_shading_position_h.w;
		const bool in_frustum = proj_shading_position_h.z > 0 && all(proj_shading_position.xy <= 1..xx) && all(proj_shading_position.xy >= -1..xx);
		const vec3 d = position - shading_position;
		const float dist2 = Length2(d);
		res.direction = Normalize(d);
		res.distance = Length(d);
		res.pdf = dist2;
		if(in_frustum)
		{
			res.extra.xy = proj_shading_position.xy;
			const float ct = -Dot(res.direction, this.direction);
			const float ct3 = ct * ct * ct;
			res.intensity = emission * (rcp(ct3 * sqr(this.tan_half_fov)) * getInvArea());
			res.intensity *= evaluateAttenuation(res.extra.xy);
		}
		else
		{
			res.intensity = 0;
		}
		res.flags = Light::Sample::Flags::DeltaPosition;
		return res;
	}

	float pdfLi(vec3 reference_position, vec3 direction)
	{
		return Length2(direction);
	}

	Light::EmissionSample sampleLe<RNG : IRNG>(inout RNG rng)
	{
		Light::EmissionSample res;
		res.ray.origin = position;
		// sample the cp that considers the attenuation
		vec2 xi = rng.generate<float, 2>();
		vec2 cp;
		float pdf = 1.0;
		const uint attenuation_flags = flags & SPOT_LIGHT_FLAG_ATTENUATION_MASK;
		if((attenuation_flags) != 0)
		{
			let sample = SampleUniformOnDisk(xi);
			cp = sample.value;
			pdf = sample.pdf;
		}
		else
		{
			cp = UVToClipSpace(xi);
			pdf = 0.25;
		}
		// Explicit z_near at 1
		// Here the z_near does not mathematically matter, 
		// but with a "usual" value like 1e-3, it makes a loss of precision in the matrix product
		// which induces some aliasing artifacts in the sampling
		res.ray.direction = Normalize(getProjToWorld(1) * vec4(cp, 1, 1));
		res.range = {};
		const float ct = Dot(res.ray.direction, this.direction);
		const float ct3 = ct * ct * ct;
		res.pdf = pdf * rcp(ct3 * sqr(this.tan_half_fov)) * getInvArea();
		res.position_pdf = 1;
		res.intensity = emission * (rcp(ct3 * sqr(this.tan_half_fov)) * getInvArea());
		res.intensity *= evaluateAttenuation(cp);
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
				const uint attenuation_flags = flags & SPOT_LIGHT_FLAG_ATTENUATION_MASK;
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

}



ILight MakeLight(const in StorageLight l)
{
	const LightType type = l.getType();
	if(type == LightType::Point)
	{
		return PointLight(l);
	}
	else if(type == LightType::Directional)
	{
		return DirectionalLight(l);
	}
	else// if(type == LightType::Spot)
	{
		return SpotLight(l);
	}
}

