#pragma once

#include "common.glsl"

#ifndef BIND_SCENE
#define BIND_SCENE 1
#endif
#ifndef LIGHTS_ACCESS
#define LIGHTS_ACCESS readonly
#endif

#include <ShaderLib:/Rendering/Scene/Scene.glsl>

#include <ShaderLib:/Rendering/Ray.glsl>

#include <ShaderLib:/Rendering/CubeMap.glsl>

#include <ShaderLib:/RayTracingCommon.glsl>

#include <ShaderLib:/Rendering/Shading/microfacets.glsl>

#include "shadingDefinitions.h"

struct LightSample
{
	vec3 Le;
	vec3 direction_to_light;
	float dist_to_light;
	float pdf;
	uint light_type;
	uint light_id;
	vec2 uv;
	float ref_depth;
};

struct GeometryShadingInfo
{
	vec3 position;
	vec3 geometry_normal;
	vec3 vertex_shading_normal;
	vec3 shading_normal;
	vec3 vertex_shading_tangent;
};

#if BIND_SCENE

PBMaterialSampleData readMaterial(uint material_id, vec2 uv)
{
	const PBMaterialProperties props = scene_pb_materials[material_id].props;
	PBMaterialSampleData res;
	res.flags = props.flags;
	res.albedo = 0..xxx;
	res.alpha = 1.0f;
	res.normal = vec3(0, 0, 1);
	const MaterialTextureIds textures = scene_pb_materials_textures.ids[material_id];
	const uint albedo_texture_id = GetMaterialTextureId(textures, ALBEDO_ALPHA_TEXTURE_SLOT);
	if(SHADING_MATERIAL_READ_TEXTURES && ((res.flags & (MATERIAL_FLAG_USE_ALBEDO_TEXTURE_BIT | MATERIAL_FLAG_USE_ALPHA_TEXTURE_BIT)) != 0) && albedo_texture_id != uint(-1))
	{
		const vec4 albedo_alpha = texture(SceneTextures2D[NonUniformEXT(albedo_texture_id)], uv);
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

#if SHADING_FORCE_MAX_NORMAL_LEVEL >= SHADING_NORMAL_LEVEL_TEXTURE
	const uint normal_texture_id = GetMaterialTextureId(textures, NORMAL_TEXTURE_SLOT);
	if(((res.flags & MATERIAL_FLAG_USE_NORMAL_TEXTURE_BIT) != 0) && normal_texture_id != uint(-1))
	{
		vec3 normal_xyz = texture(SceneTextures2D[NonUniformEXT(normal_texture_id)], uv).xyz;
		res.normal = normalize(normal_xyz * 2 - 1);
	}
#endif

#if SHADING_FORCE_WHITE_DIFFUSE
	res.metallic = 0;
	res.roughness = 1;
	res.cavity = 0;
	res.albedo = vec3(1);
#else
	res.metallic = props.metallic;
	res.roughness = props.roughness;
	res.cavity = props.cavity;
#if 0
	const float rt = 5e-2;
	if(res.roughness != 0 && res.roughness < rt)
	{
		res.roughness = rt;
	}
#endif
#endif

	return res;
}

#endif


// wo: outcoming direction
// wi: incoming direction
// Assume directions vectors are normalized
// The returned BSDF is NOT multiplied by cos_theta
vec3 evaluateBSDF(const in GeometryShadingInfo gsi, vec3 wo, vec3 wi, const in PBMaterialSampleData material)
{
	vec3 res = 0..xxx;
	
	const vec3 geometry_normal = gsi.geometry_normal;
	const vec3 shading_normal = gsi.shading_normal;
	const vec3 normal = gsi.shading_normal;

	const float cos_theta_geom_i = dot(geometry_normal, wi);
	const float cos_theta_geom_o = dot(geometry_normal, wo);

	const float cos_theta_i = dot(normal, wi);
	const float abs_cos_theta_i = abs(cos_theta_i);
	const float cos_theta_o = dot(normal, wo);
	const float abs_cos_theta_o = abs(cos_theta_o);

	const bool same_hemisphere = sign(cos_theta_geom_i) == sign(cos_theta_geom_o);

	const bool can_reflect = (material.flags & MATERIAL_FLAG_REFLECTION_BIT) != 0;
	const bool can_transmit = (material.flags & MATERIAL_FLAG_TRANSMISSION_BIT) != 0;

	if((same_hemisphere && can_reflect) || (!same_hemisphere && can_transmit))
	{

		const vec3 reflected = reflect(-wo, normal);
		const vec3 halfway = normalize(wo + wi);

		const float alpha2 = sqr(material.roughness);
		//const float alpha2 = sqr(alpha);
		const float specular_k = sqr(material.roughness + 1) / 8;

		const vec3 F0 = lerp(vec3(0.04), material.albedo, material.metallic);
		const vec3 specular_F = FresnelSchlick(F0, wo, halfway);
		const vec3 Kd = 1..xxx - specular_F; 

		const float diffuse_rho = oo_PI;
		vec3 diffuse_contribution = Kd * material.albedo * diffuse_rho * (1.0 - material.metallic);
		res += diffuse_contribution;

		//const vec3 F0 = material.F0;

		if(nonZero(F0) && (material.roughness < 1.0f || material.metallic != 0.0f))
		{
		
			const float specular_D = microfacetD(alpha2, normal, halfway);
			const float specular_G = microfacetG(normal, wo, wi, specular_k);

			const float div = max(4.0f * abs_cos_theta_i * abs_cos_theta_o, EPSILON_f);

			const vec3 specular_cook_torrance = specular_F * specular_D * specular_G / div;
			if(!any(isWrong(specular_cook_torrance)))
			{
				res += specular_cook_torrance;
			}
		}

	}

	return res;
}

#define BSDF_REFLECTION_BIT 0x1
#define BSDF_TRANSMISSION_BIT  0x2
#define BSDF_HEMISPHERE_MASK 0x3

uint shadingFaceFlags(bool front)
{
	uint res = front ? BSDF_REFLECTION_BIT : BSDF_TRANSMISSION_BIT;
	return res;
}

#if BIND_SCENE

LightSample getLightSample(uint light_id, vec3 position, vec3 normal, uint shading_flags)
{

#if SHADING_SHADOW_METHOD == SHADING_SHADOW_RAY_TRACE
	Ray ray;
	float light_dist = 0; // 0 means no ray
#endif

	LightSample res;
	const Light light = lights_buffer.lights[light_id];
	const uint light_type = light.flags & LIGHT_TYPE_MASK;
	res.light_type = light_type;
	res.light_id = light_id;
	res.pdf = 1;
	if(light_type == LIGHT_TYPE_POINT)
	{
		const vec3 to_light = light.position - position;
		const float dist2 = dot(to_light, to_light);
		const vec3 dir_to_light = normalize(to_light);
		res.Le = light.emission / dist2;
		res.direction_to_light = dir_to_light;
		res.dist_to_light = sqrt(dist2);
	}
	else if(light_type == LIGHT_TYPE_DIRECTIONAL)
	{
		const vec3 dir_to_light = light.direction;
		res.Le = light.emission;
		res.direction_to_light = dir_to_light;
		res.dist_to_light = POSITIVE_INF_f;
	}
	else if (light_type == LIGHT_TYPE_SPOT)
	{
		const mat4 light_matrix = getSpotLightMatrixWorldToProj(light);
		vec4 position_light_h = (light_matrix * vec4(position, 1));
		vec3 position_light = position_light_h.xyz / position_light_h.w;
		const vec3 to_light = light.position - position;
		res.Le = 0..xxx;

		if(position_light_h.z > 0)
		{
			const float dist2 = dot(to_light, to_light);
			const vec3 dir_to_light = normalize(to_light);
			res.direction_to_light = dir_to_light;
			res.dist_to_light = sqrt(dist2);
			const vec2 clip_uv_in_light = position_light.xy;
			if(clip_uv_in_light == clamp(clip_uv_in_light, -1.0.xx, 1.0.xx))
			{
				res.Le = light.emission / (dist2);// * position_light.z;
				res.uv = clipSpaceToUV(clip_uv_in_light);
				res.ref_depth = position_light.z;
				
				const uint attenuation_flags = light.flags & SPOT_LIGHT_FLAG_ATTENUATION_MASK;
				if((attenuation_flags) != 0)
				{
					float attenuation = 0;
					float dist_to_center = 0;
					if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_LINEAR)
					{
						dist_to_center = length(clip_uv_in_light);
					}
					else if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_QUADRATIC)
					{
						dist_to_center = length2(clip_uv_in_light);
					}
					else if(attenuation_flags == SPOT_LIGHT_FLAG_ATTENUATION_ROOT)
					{
						dist_to_center = sqrt(length(clip_uv_in_light));
					}
					
					attenuation = max(1.0 - dist_to_center, 0);
					res.Le *= attenuation;
				}
			}
		}
		else
		{
			res.Le = 0..xxx;
			res.pdf = 0;
		}
	}
	return res;
}

#endif

float applyShadowBias2(float depth, const in Light light, vec3 geometry_normal)
{
	return depth;
}

// cos_theta: abs(dot(light_forward_dir, geometry_normal))
float applyShadowBias(float depth, uint light_flags, uint bias_data, float cos_theta)
{
	const uint bias_mode = light_flags & LIGHT_SHADOW_MAP_BIAS_MODE_MASK;
	const bool include_theta = (light_flags & LIGHT_SHADOW_MAP_BIAS_INCLUDE_THETA_BIT) != 0;
	float res = depth;

	if(bias_mode == LIGHT_SHADOW_MAP_BIAS_MODE_OFFSET)
	{
		const int offset = int(bias_data);
		res = floatOffset(depth, -offset);
	}
	else if(bias_mode == LIGHT_SHADOW_MAP_BIAS_MODE_FLOAT_MULT)
	{
		float m = uintBitsToFloat(bias_data);
		if(include_theta)
		{
			m = lerp(1, m*m, cos_theta);
		}
		res = depth * m;
	}
	else if(bias_mode == LIGHT_SHADOW_MAP_BIAS_MODE_FLOAT_ADD)
	{
		const float b = uintBitsToFloat(bias_data);
		res = depth + b;
	}

	return res;
}

#if BIND_SCENE

#if SHADER_RAY_QUERY_AVAILABLE

void ProcessRayQueryCandidateIntersection(RayQuery_t rq)
{
	const uint ray_flags = rayQueryGetRayFlagsEXT(rq);
	if(((ray_flags & gl_RayFlagsSkipAABBEXT) != 0) || (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT))
	{
		const uint custom_index = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false);
		const uint instance_index = rayQueryGetIntersectionInstanceIdEXT(rq, false);
		const uint sbt_index = rayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetEXT(rq, false);
		const uint geometry_index = rayQueryGetIntersectionGeometryIndexEXT(rq, false);
		const uint object_index = custom_index;
		const int primitive_id = rayQueryGetIntersectionPrimitiveIndexEXT(rq, false);
		const vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rq, false);
		const mat4x3 object_to_world = rayQueryGetIntersectionObjectToWorldEXT(rq, false);
		const mat3 object_to_world_4dir = DirectionMatrix(mat3(object_to_world));

		if(SceneTestTriangleOpacity(object_index, primitive_id, barycentrics))
		{
			rayQueryConfirmIntersectionEXT(rq);
		}
	}
}


void TraceSceneRayQuery(RayQuery_t rq, const in Ray ray, vec2 range, uint extra_ray_flags)
{
	uint ray_flags = gl_RayFlagsSkipAABBEXT | extra_ray_flags;
#if (!SHADING_MATERIAL_READ_TEXTURES) ||(!SHADING_ENABLE_OPACITY_TEST) 
	ray_flags |= gl_RayFlagsOpaqueEXT;
#endif
	const uint cull_mask = 0xFF;
	rayQueryInitializeEXT(rq, SceneTLAS, ray_flags, cull_mask, ray.origin, range.x, ray.direction, range.y);
	while(rayQueryProceedEXT(rq))
	{
		ProcessRayQueryCandidateIntersection(rq);
	}
}

void TraceSceneRayQuery(RayQuery_t rq, const in Ray ray, vec2 range)
{
	TraceSceneRayQuery(rq, ray, range, 0);
}

bool RayQuerySceneVisibility(const in Ray ray, vec2 range, uint extra_ray_flags)
{
	
	RayQuery_t rq;
	TraceSceneRayQuery(rq, ray, range, gl_RayFlagsTerminateOnFirstHitEXT | extra_ray_flags);
	bool res = (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT);
	return res;	
}

bool RayQuerySceneVisibility(const in Ray ray, vec2 range)
{
	return RayQuerySceneVisibility(ray, range, 0);
}



#endif

// cos_theta: abs(dot(light_forward_dir, geometry_normal))
float computeShadow(vec3 position, vec3 geometry_normal, const in LightSample light_sample)
{
	float res = 1;
#if SHADING_SHADOW_METHOD == SHADING_SHADOW_MAP
	const Light light = lights_buffer.lights[light_sample.light_id];
	const uint light_type = light.flags & LIGHT_TYPE_MASK;
	const uint shadow_texture_index = light.textures.x;
	bool query_shadow_map = ((light.flags & LIGHT_ENABLE_SHADOW_MAP_BIT) != 0) && (shadow_texture_index != uint(-1));
	if(query_shadow_map)
	{
		if(light_type == LIGHT_TYPE_POINT)
		{
			const vec4 ref_direction_depth = CubeMapDirectionAndDepth(position, light.position, getLightZNear(light));
			float ref_depth = ref_direction_depth.w;
			const vec3 light_main_direction = ref_direction_depth.xyz;
			// Could also compute the sin_theta with the norm of the cross produc
			const float cos_theta = adot(light_main_direction, geometry_normal);
			const float sin_theta = sqrt(1 - sqr(cos_theta));
			const ivec2 tex_size = textureSize(samplerCubeShadow(LightsDepthCube[shadow_texture_index], LightDepthSampler), 0);
			
			const float shadow_texel_size = sin_theta * ref_depth / tex_size.x;
			vec2 lerp_range = vec2(4096, 128);
			int offset = int(lerp(lerp_range.x, lerp_range.y,shadow_texel_size ));
			offset = int(1024 * (1 + sin_theta));
			ref_depth = floatOffset(ref_depth, -offset);

			//ref_depth = applyShadowBias(ref_depth, light.flags, light.shadow_bias_data, cos_theta);
			float texture_depth = texture(samplerCubeShadow(LightsDepthCube[shadow_texture_index], LightDepthSampler), vec4(-light_sample.direction_to_light, ref_depth));

			res = texture_depth;	
		}
		else if(light_type == LIGHT_TYPE_DIRECTIONAL)
		{
			// TODO
		}
		else if(light_type == LIGHT_TYPE_SPOT)
		{
			vec2 light_tex_uv = light_sample.uv;
			const float ref_depth = light_sample.ref_depth;
			const vec3 light_main_direction = light.direction;
			const float cos_theta = adot(light_main_direction, geometry_normal);
			const float sin_theta = sqrt(1 - sqr(cos_theta));
			const ivec2 tex_res = textureSize(sampler2DShadow(LightsDepth2D[shadow_texture_index], LightDepthSampler), 0);
			const float tex_size = float(tex_res.x);
			const float shadow_texel_size = sin_theta * (ref_depth) / (log2(tex_size));
			vec2 lerp_range = vec2(16, 128 * 8);
			int offset = int(lerp(lerp_range.x, lerp_range.y, shadow_texel_size));
			offset = int(64 * (1 + sin_theta));
			const float biased_ref_depth = floatOffset(ref_depth, -offset);

			//ref_depth = applyShadowBias(ref_depth, light.flags, light.shadow_bias_data, cos_theta);
			const float texture_depth_test = texture(sampler2DShadow(LightsDepth2D[shadow_texture_index], LightDepthSampler), vec3(light_tex_uv, biased_ref_depth));
			res = texture_depth_test;
			//const float texture_depth = texture(sampler2D(LightsDepth2D[shadow_texture_index], LightDepthSampler), vec2(light_tex_uv)).x;
			//res = (biased_ref_depth <= texture_depth) ? 1 : 0;
		}
	}
#elif SHADING_SHADOW_METHOD == SHADING_SHADOW_RAY_TRACE
	Ray ray = Ray(position, light_sample.direction_to_light);
	const float t_min = rayTMin(ray, geometry_normal);
	float t_max = light_sample.dist_to_light;
	if(light_sample.light_type == LIGHT_TYPE_SPOT)
	{
		//t_max = 0.1;
	}
	bool v = RayQuerySceneVisibility(ray, vec2(t_min, t_max));
	res = v ? 1.0f : 0.0f;
#endif
	return res;
}

vec3 shade(const in GeometryShadingInfo geom, vec3 wo, const in PBMaterialSampleData material)
{
	vec3 res = 0..xxx;

	for(uint l = 0; l < scene_ubo.num_lights; ++l)
	{
		const LightSample light_sample = getLightSample(l, geom.position, geom.geometry_normal, BSDF_REFLECTION_BIT);
		const vec3 wi = light_sample.direction_to_light;
		if(light_sample.pdf > 0)
		{
			const float cos_theta = dot(geom.shading_normal, wi);
			const float abs_cos_theta = abs(cos_theta);
			const vec3 bsdf_rho = evaluateBSDF(geom, wo, wi, material);

			float shadow = 1;
			if(length2(light_sample.Le * bsdf_rho) > 0)
			{
				// TODO geometry normal
				shadow = computeShadow(geom.position, geom.geometry_normal, light_sample);
			}
			res += (bsdf_rho * abs_cos_theta * light_sample.Le * shadow) / light_sample.pdf; 
		}
	}

	// {
	// 	vec2 uv = (vec2(gl_GlobalInvocationID.xy) + 0.5) / vec2(2731, 1500);
	// 	//float texture_depth = textureLod(sampler2DShadow(LightsDepth2D[0], LightDepthSampler), vec3(uv, 0.99995), 0).x;
	// 	float texture_depth = textureLod(sampler2D(LightsDepth2D[0], LightDepthSampler), vec2(uv), 0).x;
	// 	float range = 1e-4;
	// 	texture_depth = (texture_depth - (1.0 - range)) / range;
	// 	res = texture_depth.xxx;
	// }

	return res;
}


#endif


bool TestOpacity(float sampled_alpha)
{
	return sampled_alpha >= GetSceneOpaqueAlphaThreshold();
}
