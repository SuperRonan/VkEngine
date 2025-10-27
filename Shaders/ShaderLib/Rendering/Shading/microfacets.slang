#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Random.slang>
#include <ShaderLib/Maths/Transforms.slang>

// https://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf

namespace microfacets
{
	// Normal distribution "D"
	__generic <Float : __BuiltinFloatingPointType>
	Float NormalDistribution(Float alpha2, Vector3<Float> normal, Vector3<Float> halfway)
	{
		return alpha2 / (Float(M_PI) * sqr(sqr(dot(normal, halfway)) * (alpha2 - Float(1)) + Float(1)));
	}

	__generic <Float : __BuiltinFloatingPointType>
	Float GGX(Vector3<Float> n, Vector3<Float> v, Float k)
	{
		const Float d = max(dot(n, v), Float(0));
		Float res = d / (d * (Float(1) - k) + k);
		//res = abs(res);
		//res = max(res, 0);
		return res;
	}
	
	// Shadowing masking function "G"
	__generic <Float : __BuiltinFloatingPointType>
	Float GeometryShadowMasking(Vector3<Float> n, Vector3<Float> wo, Vector3<Float> wi, Float k)
	{
		const Float gi = GGX(n, wi, k);
		const Float go = GGX(n, wo, k);
		Float res;
		res = gi * go;
		//res = max(res, 0);
		return res;
	}
	
	__generic <Float : __BuiltinFloatingPointType>
	Float SpecularFresnelNormalIncidence(Float n)
	{
		return sqr(n - Float(1)) / sqr(n + Float(1));
	}
	
	__generic <Res : IFloat, Float : __BuiltinFloatingPointType>
	Res FresnelSchlick(Res F0, Vector3<Float> v, Vector3<Float> h, bool use_approx=false)
	{
		const Float d = dot(v, h);
		const Float cos_5 = pow(Float(1) - d, Float(5));
		const Float cos_5_approx = pow(Float(2), (Float(-5.55473) * d + Float(-6.98316)) * d);
		return F0.add((Res(1) - F0).scale(use_approx ? cos_5_approx : cos_5));
	}

	struct Approximation
	{
		float specular_weight;
		float shininess;

		__init(float cos_theta_o, float roughness, float metallic)
		{
			// if(roughness == 0 && metallic == 0)
			// {
		
			// }
			// else
			{
				specular_weight = max(metallic, 0.0f);
			}
			if(roughness == 0)
			{
				//res.x = 1.0f / 0.0f;
				shininess = 1;
			}
			else
			{
				// const float soft_limit = 0.2;
				// if(roughness < (soft_limit))
				// {
				// 	//roughness = lerp(soft_limit * 0.35, soft_limit, roughness / soft_limit);
				// }
				const float limit = 2e-4;
				if(roughness < limit)
				{
					roughness = limit;
				}
				shininess = pow(roughness, -2);
				//res.x = rcp(sqr(sqr(roughness)));
				//res.x = clamp(res.x, 1, 1e5);
			}
		}

		float evaluate(vec3 center_direction, vec3 direction)
		{
			const float m = (shininess + 1) / TWO_PI;
			const float c = max(0, dot(normalize(center_direction), normalize(direction)));
			const float p = pow(c, shininess);
			return m * p;
		}

		// Generzte a direction centered on the Z direction
		vec3 generateRandomDirection<RNG : IRNG>(inout RNG rng)
		{
			const float2 xy = rng.generate<float, 2>();
			const float p = pow(xy.x, rcp(shininess + 1));
			const float theta = acos(p);
			const float phi = TWO_PI * xy.y;
			vec3 res = SphericalToCartesian(vec2(theta, phi)).xzy;
			return res;
		}
	};
}



