#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/View.slang>
#include <ShaderLib/Random.slang>

#include <ShaderLib/Maths/NumericLimits.slang>


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
		float pdf;

		vec3 direction;
		Flags flags;

		float distance;
		// Optional additional fields
		vec3 extra;
	};
};

interface ILight
{
	__init(const in StorageLight l);

	uint getFlags();

	LightType getType();

	// Directions can be zero (means no direction)
	Light::Sample sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position, vec3 shading_normal, vec3 wo);
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

	Light::Sample sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position, vec3 shading_normal, vec3 wo)
	{
		Light::Sample res;
		const vec3 d = position - shading_position;
		res.direction = Normalize(d);
		res.intensity = emission / Length2(d);
		res.pdf = 1;
		res.flags = Light::Sample::Flags::DeltaPosition;
		res.distance = Length(d);
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

	Light::Sample sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position, vec3 shading_normal, vec3 wo)
	{
		Light::Sample res;
		res.direction = direction;
		res.intensity = emission;
		res.pdf = 1;
		res.flags = Light::Sample::Flags::AtInfinity || Light::Sample::Flags::DeltaDirection;
		res.distance = NumericLimits<float>::infinity();
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

	Matrix4f getProj()
	{
		return InfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near) * ResizeMatrix<4, 4>(LookAtDirAssumeOrtho(position, direction, up));
	}

	Light::Sample sampleLi<RNG : IRNG>(inout RNG rng, vec3 shading_position, vec3 shading_normal, vec3 wo)
	{
		Light::Sample res;
		const vec4 proj_shading_position_h = getProj() * Homogeneous(shading_position);
		const vec3 proj_shading_position = proj_shading_position_h.xyz / proj_shading_position_h.w;
		const bool in_frustum = proj_shading_position_h.z > 0 && all(proj_shading_position.xy <= 1..xx) && all(proj_shading_position.xy >= -1..xx);
		if(in_frustum)
		{
			const vec3 d = position - shading_position;
			res.direction = Normalize(d);
			const float dist2 = Length2(d);
			res.intensity = emission / dist2;
			res.extra.xy = proj_shading_position.xy;
			res.pdf = 1;
			res.distance = Length(d);

			const uint attenuation_flags = flags & SPOT_LIGHT_FLAG_ATTENUATION_MASK;
			if((attenuation_flags) != 0)
			{
				float attenuation = 0;
				float dist_to_center = 0;
				if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_LINEAR)
				{
					dist_to_center = length(proj_shading_position.xy);
				}
				else if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_QUADRATIC)
				{
					dist_to_center = Length2(proj_shading_position.xy);
				}
				else if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_ROOT)
				{
					dist_to_center = sqrt(length(proj_shading_position.xy));
				}
				
				attenuation = max(1.0 - dist_to_center, 0);
				res.intensity *= attenuation;
			}


		}
		else
		{
			res.intensity = 0;
			res.pdf = 1;	
		}
		res.flags = Light::Sample::Flags::DeltaPosition;
		return res;
	}

}



bool MakeLight(const in StorageLight l, out ILight res)
{
	bool ret_code = false;
	const LightType type = l.getType();
	if(type == LightType::Point)
	{
		res = PointLight(l);
		ret_code = true;
	}
	else if(type == LightType::Directional)
	{
		res = DirectionalLight(l);
		ret_code = true;
	}
	else if(type == LightType::Spot)
	{
		res = SpotLight(l);
		ret_code = true;
	}
	return ret_code;
}

