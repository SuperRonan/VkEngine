#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/View.slang>
#include <ShaderLib/Maths/Sampling.slang>
#include <ShaderLib/Rendering/CubeMap.slang>
#include <ShaderLib/Random.slang>

#include <ShaderLib/Maths/NumericLimits.slang>

#include <ShaderLib/Rendering/Ray.slang>

#include "Definitions.h"

#include <ShaderLib/Spectrum/Spectrum.slang>

enum LightType
{
	Point = LIGHT_TYPE_POINT,
	Directional = LIGHT_TYPE_DIRECTIONAL,
	Spot = LIGHT_TYPE_SPOT,
	Beam = LIGHT_TYPE_BEAM,
	EnvMap,
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

	float getBeamRadius()
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
	};

	struct ConnectionSample<int Mode>
	{
		typealias Flags = Sample::Flags;
		
		Spectrumf<Mode> intensity;
		PackedEnumIndex<Flags, 8, u32> flags_index;
		float pdf = 0;

		vec3 direction;

		float distance;
		// Optional additional fields
		vec2 extra;

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
	
	struct EmissionSample<int Mode>
	{
		typealias Flags = Sample::Flags;

		Spectrumf<Mode> intensity;
		PackedEnumIndex<Flags, 8, u32> flags_index;
		float pdf;

		Ray3f ray;
		RayRangef range;
		float position_pdf;

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

	static float EvaluateAttenuation(uint flags, vec2 cp)
	{
		float res = 1;
		const uint attenuation_flags = flags & LIGHT_FLAG_ATTENUATION_MASK;
		if((attenuation_flags) != 0)
		{
			if(attenuation_flags == LIGHT_FLAG_ATTENUATION_INSIDE)
			{
				if(Length2(cp) > 1.0f)
				{
					res = 0;
				}
			}
			else
			{
				float dist_to_center = 0;
				if(attenuation_flags == LIGHT_FLAG_ATTENUATION_LINEAR)
				{
					dist_to_center = length(cp);
				}
				else if(attenuation_flags == LIGHT_FLAG_ATTENUATION_QUADRATIC)
				{
					dist_to_center = Length2(cp);
				}
				else if(attenuation_flags == LIGHT_FLAG_ATTENUATION_INSIDE)
				{
					dist_to_center = sqrt(length(cp));
				}
				res = max(1.0 - dist_to_center, 0);
			}
		}
		return res;
	}

	static Spectrumf<Mode> GetSpectralEmission<int Mode>(vec3 emission, bool black_body)
	{
		Spectrumf<Mode> res;
		if(black_body)
		{
			float temperature = emission.x;
			float intensity = emission.y;
			res = SpectrumFromTemperature<Mode>(temperature, true) * intensity;
		}
		else
		{
			res = Spectrumf<Mode>::MakeFromRGB(emission, RGBSourceType::Illuminant);
		}
		return res;
	}
	
	static Spectrumf<Mode> GetSpectralEmission<int Mode>(vec3 emission, uint flags)
	{
		bool black_body = (flags & LIGHT_BLACK_BODY_EMISSION_BIT_FLAG) != 0;
		return GetSpectralEmission<Mode>(emission, black_body);
	}
};

interface ILight<int Mode>
{
	__init(const in StorageLight l);

	uint getFlags();

	LightType getType();

	// Directions can be zero (means no direction)
	Light::ConnectionSample<Mode> sampleLi<RNG : IRNG>(inout RNG rng, vec3 reference_position);
	
	// Direction not normalized
	// Assume reference_position + direction is on the light
	float pdfLi(vec3 reference_position, vec3 direction);
	
	Light::EmissionSample<Mode> sampleLe<RNG : IRNG>(inout RNG rng);

	RayPdf pdfLe(Ray3f ray);
};
