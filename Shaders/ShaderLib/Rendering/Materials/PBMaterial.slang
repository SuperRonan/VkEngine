#pragma once

#include <ShaderLib/Debug/DebugBuffers.slang>

#include "Material.slang"

#include <ShaderLib/random.slang>

#include "PBMaterialDefinitions.h"

#include <ShaderLib/Rendering/Shading/microfacets.slang>

#include <ShaderLib/Maths/3DMatrices.slang>
#include <ShaderLib/Maths/Calculus.slang>
#include <ShaderLib/Maths/Sampling.slang>



struct PhysicalyBasedMaterialProperties
{
	vec3 albedo;
	uint flags;
	
	float metallic;
	float roughness;
	float cavity;

	__init()
	{
		albedo = vec3(0);
		flags = 0;
		metallic = 0;
		roughness = 1;
		cavity = 0;
	}
}

typealias PBMaterialProperties = PhysicalyBasedMaterialProperties;

struct PhysicalyBasedMaterial : IMaterial
{
	vec3 albedo;
	float alpha;
	
	vec3 normal;
	uint flags;
	
	float metallic;
	float roughness;
	float cavity;
	
	__init()
	{
		albedo = vec3(0);
		alpha = 1;
		normal = vec3(0, 0, 1);
		flags = 0;
		metallic = 0;
		roughness = 1;
		cavity = 0;
	}

	__init(const in PhysicalyBasedMaterialProperties props)
	{
		albedo = props.albedo;
		alpha = 1;
	
		normal = vec3(0, 0, 1);
		flags = props.flags;

		metallic = props.metallic;
		roughness = props.roughness;
		cavity = props.cavity;
	}

	uint getFlags()
	{
		return flags;
	}

	bool isDielectric()
	{
		return (flags & PB_MATERIAL_DIELECTRIC_BIT) != 0;
	}

	bool isMicrofacets()
	{
		return !isDielectric();
	}

	MaterialType getType()
	{
		return MaterialType::PhysicalyBased;
	}

	float3 bsdf_Microfacets<int Flags>(
		const in vec3 geometry_normal,
		const in vec3 shading_normal,
		const in vec3 wo,
		const in vec3 wi
	) {
		vec3 res = 0..xxx;

		const vec3 normal = shading_normal;

		const float cos_theta_geom_i = dot(geometry_normal, wi);
		const float cos_theta_geom_o = dot(geometry_normal, wo);

		const float cos_theta_i = dot(normal, wi);
		const float abs_cos_theta_i = abs(cos_theta_i);
		const float cos_theta_o = dot(normal, wo);
		const float abs_cos_theta_o = abs(cos_theta_o);

		// TODO really check this
		const bool same_hemisphere = sign(cos_theta_i) == sign(cos_theta_o);

		const bool can_reflect = (getFlags() & MATERIAL_FLAG_REFLECTION_BIT) != 0;
		const bool can_transmit = (getFlags() & MATERIAL_FLAG_TRANSMISSION_BIT) != 0;

		if((same_hemisphere && can_reflect) || (!same_hemisphere && can_transmit))
		{
			const vec3 reflected = reflect(-wo, normal);
			const vec3 halfway = normalize(wo + wi);

			const float alpha2 = sqr(roughness);
			//const float alpha2 = sqr(alpha);
			const float specular_k = sqr(roughness + 1) / 8;

			const vec3 F0 = lerp(vec3(0.04), albedo, metallic);
			const vec3 specular_F = microfacets::FresnelSchlick(F0, wo, halfway);
			const vec3 Kd = 1..xxx - specular_F; 

			const float diffuse_rho = oo_PI;
			vec3 diffuse_contribution = Kd * albedo * diffuse_rho * (1.0 - metallic);
			res += diffuse_contribution;

			//const vec3 F0 = material.F0;

			if(NonZero(F0) && (roughness < 1.0f || metallic != 0.0f))
			{
				const float specular_D = microfacets::NormalDistribution(alpha2, normal, halfway);
				const float specular_G = microfacets::GeometryShadowMasking(normal, wo, wi, specular_k);

				const float div = max(4.0f * abs_cos_theta_i * abs_cos_theta_o, EPSILON_f);

				const vec3 specular_cook_torrance = specular_F * specular_D * specular_G / div;
				if(!any(IsWrong(specular_cook_torrance)))
				{
					res += specular_cook_torrance;
				}
			}
		}

		return res;
	}

	float3 bsdf_Dielectric<int Flags>(
		const in vec3 geometry_normal,
		const in vec3 shading_normal,
		const in vec3 wo,
		const in vec3 wi
	) {
		vec3 res = 0..xxx;
		return res;
	}

	float3 bsdf<int Flags>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in vec3 wo, 
		const in vec3 wi
	) {
		if(isDielectric())
		{
			return this.bsdf_Dielectric<Flags>(geometry_normal, shading_normal, wo, wi);
		}
		else
		{
			return this.bsdf_Microfacets<Flags>(geometry_normal, shading_normal, wo, wi);
		}
	}

	float pdf_Microfacets<int Flags>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in vec3 wo, 
		const in vec3 wi
	) {
		const vec3 oriented_normal = (shading_normal);
		const vec3 normal = dot(oriented_normal, wo) < 0 ? -oriented_normal : oriented_normal;
		const mat3 local_to_world_basis = BasisFromDir(normal);
		const vec3 reflected = Reflect(wo, normal);

		const float cos_theta_o = dot(normal, wo);

		microfacets::Approximation approx = microfacets::Approximation(cos_theta_o, this.roughness, this.metallic);

		const bool has_dielectric = this.roughness == 0.0f;

		const bool force_cosine_sampling = bool(false);
		if(force_cosine_sampling)
		{
			approx.specular_weight = 0.0f;
		}

		float cosine_pdf = Dot(wi, normal) * oo_PI;
		float specular_pdf = 0.0f;
		if(this.roughness > 0.0f)
		{
			specular_pdf = approx.evaluate(reflected, wi);
		}
		float res = lerp(cosine_pdf, specular_pdf, approx.specular_weight);

		return max(res, 0);
	}

	float pdf_Dielectric<int Flags>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in vec3 wo, 
		const in vec3 wi
	) {
		return 0;
	}

	float pdf<int Flags>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in vec3 wo, 
		const in vec3 wi
	) {
		if(isDielectric())
		{
			return this.pdf_Dielectric<Flags>(geometry_normal, shading_normal, wo, wi);
		}
		else
		{
			return this.pdf_Microfacets<Flags>(geometry_normal, shading_normal, wo, wi);
		}
	}

	BSDFSample<N> sampleBSDF_Microfacets<int Flags, typename RNG : IRNG, int N>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in Matrixf<3, N> Jnormal,
		const in vec3 wo, 
		const in Matrixf<3, N> Jwo,
		inout RNG rng, 
		uint flags = 0)
	{
		BSDFSample<N> res = {};
		res.pdf = 1;

		const vec3 normal = dot(geometry_normal, wo) < 0 ? -shading_normal : shading_normal;
		const mat3 local_to_world_basis = BasisFromDir(normal);
		const vec3 reflected = Reflect(wo, normal);

		const float cos_theta_o = dot(normal, wo);

		const vec3 F0 = lerp(vec3(0.04), this.albedo, this.metallic);

		microfacets::Approximation approx = microfacets::Approximation(cos_theta_o, this.roughness, this.metallic);

		const bool has_dielectric = this.roughness == 0.0f;

		const bool force_cosine_sampling = bool(false);
		if(force_cosine_sampling)
		{
			approx.specular_weight = 0.0f;
		}
		const float specular_selection = rng.generate<float>(0, 1);
		const bool sample_specular = specular_selection < approx.specular_weight;

		if(sample_specular && has_dielectric)
		{
			res.direction = reflected;
			res.pdf = approx.specular_weight;
			res.bsdf = F0 / cos_theta_o;
			res.direction_jacobian = ReflectionDifferentials(wo, normal, Jwo, Jnormal);
			res.flags = res.flags | BSDFSample<N>::Flags::Delta;
		}
		else
		{
			float cosine_pdf = 0.0f;
			float specular_pdf = 0.0f;
			
			const mat3 reflected_basis = BasisFromDir(reflected);

			if(sample_specular && !force_cosine_sampling)
			{
				const vec3 local_direction = approx.generateRandomDirection(rng);
				res.direction = reflected_basis * local_direction;
				specular_pdf = max(0.0f, approx.evaluate(reflected, res.direction));
				cosine_pdf = max(0.0f, dot(res.direction, normal)) * oo_PI;
			}
			else
			{
				const Sample<vec3> local_direction = SampleCosineDirection(rng.generate<float, 2>());
				res.direction = local_to_world_basis * local_direction.value;
				specular_pdf = max(0.0f, approx.evaluate(reflected, res.direction));
				cosine_pdf = local_direction.pdf;
				
			}
			res.bsdf = bsdf_Microfacets<Flags>(geometry_normal, shading_normal, wo, res.direction);
			res.pdf = lerp(cosine_pdf, specular_pdf, approx.specular_weight);
		}
		res.flags = res.flags | BSDFSample<N>::Flags::Symmetric;
		return res;
	}

	BSDFSample<N> sampleBSDF_Dielectric<int Flags, typename RNG : IRNG, int N>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in Matrixf<3, N> Jnormal,
		const in vec3 wo, 
		const in Matrixf<3, N> Jwo,
		inout RNG rng, 
		uint flags = 0)
	{
		BSDFSample<N> res = {};
		res.pdf = 1;
		res.bsdf = vec3(1);
		res.flags = BSDFSampleFlags::Delta;

		bool outside = dot(geometry_normal, wo) > 0;

		float base_ior = metallic;
		const bool sample_spectral = (this.flags & PB_MATERIAL_DIELECTRIC_SPECRAL_APPROX_BIT) != 0;
		// Very basic approximation of spectral rendering effects
		if(sample_spectral)
		{
			uint channel = rng.generate<uint>(0, 3);
			res.bsdf = vec3(0);
			res.bsdf[channel] = 1;
			float deviation = float(channel) - 1.0;
			base_ior *= (1.0 + deviation * 0.01);
			res.pdf *= rcp(3);
		}
		
		float ior = outside ? rcp(base_ior) : (base_ior);
		
		// normal in the same hemisphere as wo
		const vec3 normal = outside ? shading_normal : -shading_normal;
		const vec3 g_normal = outside ? geometry_normal : -geometry_normal;
		const mat3 local_to_world_basis = BasisFromDir(normal);
		const vec3 reflected = Reflect(wo, normal);
		const vec3 refracted = Refract(wo, normal, ior);
		
		const float cos_theta_o = dot(normal, wo);
		const float sin_theta_o = sqrt(1 - sqr(cos_theta_o));

		float sin_theta_i = ior * sin_theta_o;
		bool reflect = sin_theta_i > 1;
		float refl_fresnel = 1;

		// if(_g_debug_signal)
		// {
		// 	_g_debug_printer.print(sin_theta_o);
		// 	_g_debug_printer.print(sin_theta_i);
		// }

		if(!reflect)
		{
			float cos_theta_i = sqrt(1 - sqr(sin_theta_i));
			float rs = ((ior * cos_theta_o - cos_theta_i) / (ior * cos_theta_o + cos_theta_i));
			float rp = ((cos_theta_o - ior * cos_theta_i) / (cos_theta_o + ior * cos_theta_i));
			refl_fresnel = 0.5 * (sqr(rs) + sqr(rp));

			float xi = rng.generate<float>();
			reflect = xi < refl_fresnel;
		}

		if(reflect)
		{
			res.direction = reflected;
			res.bsdf *= refl_fresnel;
			res.pdf = refl_fresnel;
			res.direction_jacobian = ReflectionDifferentials(wo, normal, Jwo, Jnormal);

			if(dot(res.direction, g_normal) < 0)
			{
				res.bsdf = 0;
			}
		}
		else
		{
			res.direction = refracted;
			res.bsdf *= (1 - refl_fresnel);
			res.pdf *= (1 - refl_fresnel);
			res.direction_jacobian = RefractionDifferentials(wo, normal, Jwo, Jnormal, refracted, ior);

			// https://www.pbr-book.org/3ed-2018/Light_Transport_III_Bidirectional_Methods/The_Path-Space_Measurement_Equation#x3-Non-symmetryDuetoRefraction
			if((Flags & BSDF_ADJOINT_BIT) != 0)
			{
				res.bsdf *= sqr(ior);
			}
			
			if(dot(res.direction, g_normal) > 0)
			{
				res.bsdf = 0;
			}
		}
		

		res.bsdf *= rcp(AbsDot(shading_normal, res.direction));

		return res;
	}



	BSDFSample<N> sampleBSDF<int Flags, typename RNG : IRNG, int N>(
		const in vec3 geometry_normal, 
		const in vec3 shading_normal, 
		const in Matrixf<3, N> Jnormal,
		const in vec3 wo, 
		const in Matrixf<3, N> Jwo,
		inout RNG rng, 
		uint flags = 0)
	{
		BSDFSample<N> res;
		if(isDielectric())
		{
			res = this.sampleBSDF_Dielectric<Flags>(geometry_normal, shading_normal, Jnormal, wo, Jwo, rng, flags);
		}
		else
		{
			res = this.sampleBSDF_Microfacets<Flags>(geometry_normal, shading_normal, Jnormal, wo, Jwo, rng, flags);
		}
		return res;
	}

	// Absorbtion factor
	Optional<vec3> getKappa(vec3 geometry_normal, vec3 wo)
	{
		Optional<vec3> res = none;
		if(isDielectric())
		{
			if(Dot(geometry_normal, wo) < 0)
			{
				res = albedo;
			}
		}
		return res;
	}
}

typealias PBMaterial = PhysicalyBasedMaterial;

#if BIND_SINGLE_MATERIAL

layout(INVOCATION_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 0)
uniform PBMaterialProperties BoundMaterial_Properties;

layout(INVOCATION_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 1) uniform Sampler2D BoundMaterial_AlbedoTexture;
layout(INVOCATION_DESCRIPTOR_BINDING + MATERIAL_BINDING_BASE + 2) uniform Sampler2D BoundMaterial_NormalTexture;

#endif

#if BIND_SINGLE_MATERIAL
PhysicalyBasedMaterial ReadBoundMaterial(vec2 uv, bool read_textures = true)
{
	const PBMaterialProperties props = BoundMaterial_Properties;
	PhysicalyBasedMaterial res = PhysicalyBasedMaterial(props);
	if(read_textures && ((res.flags & (MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT | MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT)) != 0))
	{
		const vec4 albedo_alpha = BoundMaterial_AlbedoTexture.Sample(uv);
		if((res.flags & MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT) != 0)
		{
			res.albedo = albedo_alpha.rgb;
		}
		if((res.flags & MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT) != 0)
		{
			res.alpha = albedo_alpha.a;
		}
	}
	else
	{
		res.albedo = props.albedo;
	}

	if(read_textures && ((res.flags & MATERIAL_FLAG_USE_NORMAL_TEXTURE_BIT) != 0))
	{
		res.normal = BoundMaterial_NormalTexture.Sample(uv).xyz;
		res.normal = normalize(res.normal * 2 - 1);
	}

	res.metallic = props.metallic;
	res.roughness = props.roughness;
	res.cavity = props.cavity;

	return res;
}

#endif

