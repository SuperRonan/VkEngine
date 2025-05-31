#pragma once

#include "common.slang"

#ifndef BIND_SCENE
#define BIND_SCENE 1
#endif
#ifndef SCENE_LIGHTS_ACCESS
#define SCENE_LIGHTS_ACCESS ROM 
#endif

#include <ShaderLib/Rendering/Scene/Scene.slang>

#include <ShaderLib/Rendering/CubeMap.slang>

#include "shadingDefinitions.h"

bool ApplyMaterialNormalIFN(inout SurfaceGeometryInfo geom, const in PBMaterial material)
{
	bool res = false;
	if((material.flags & PB_MATERIAL_SAMPLED_NORMAL_TEXTURE) != 0)
	{
		mat3 tbn = geom.getShadingTBN();
		geom.shading_normal = tbn * material.normal;
		res = true;
	}
	else if((material.flags & MATERIAL_FLAG_USE_GEOMETRY_NORMAL) != 0)
	{
		geom.shading_normal = geom.geometry_normal;
		res = true;
	}
	return res;
}

interface IDirectLightSurfaceShading
{
	vec3 shade<RNG : IRNG>(
		const in SurfaceGeometryInfo sgi,
		const in IMaterial material,
		inout RNG rng
	);
};

struct DirectLightShadingBase
{
	BoundScene scene;


	vec3 shadeLight<RNG : IRNG>(
		const SurfaceGeometryInfo sgi,
		const in IMaterial material,
		const in ILight light, 
		inout RNG rng
	) {
		vec3 res = 0;
		Light::ConnectionSample sample = light.sampleLi(rng, sgi.position);
		if(sample.pdf > 0)
		{
			const vec3 bsdf = material.bsdf<BSDF_FORWARD_BIT>(sgi.geometry_normal, sgi.shading_normal, sgi.out_direction, sample.direction);
			res = max(sample.intensity, vec3(0)) * bsdf * abs(dot(sgi.shading_normal, sample.direction)) / sample.pdf;
			if(NonZero(res))
			{
				RayDesc ray;
				ray.Origin = sgi.position;
				ray.Direction = sample.direction;
				ray.TMin = RayTMin(ray.getRay(), sgi.geometry_normal);
				ray.TMax = sample.distance;
				float v = scene.testVisibility(ray);
				res *= v;
			}
		}
		return res;
	}
};


struct DirectLightSurfaceShadingEachLight : DirectLightShadingBase, IDirectLightSurfaceShading
{

	uint sample_per_light = 1;

	vec3 shade <RNG : IRNG>(
		const in SurfaceGeometryInfo sgi,
		const in IMaterial material,
		inout RNG rng
	)
	{
		vec3 res = vec3(0);
		const uint num_lights = scene.getUBO().num_lights;
		for(uint i = 0; i < num_lights; ++i)
		{
			ILight l = MakeLight(SceneLights[i]);
			vec3 light_contrib = vec3(0);
			for(uint s = 0; s < sample_per_light; ++s)
			{
				light_contrib += FitWrongToZero(shadeLight(sgi, material, l, rng));
			}
			res += (light_contrib / float(sample_per_light));
		}
		return res;
	}


};


struct DirectShadingResult
{
	vec3 L = 0;
	
	__init() {}

	[mutating]
	void add(const in DirectShadingResult other)
	{
		L += other.L;
	}
}

DirectShadingResult ShadeLight(
	const in BoundScene scene,
	const in SurfaceGeometryInfo sgi,
	const in PBMaterial material,
	const in StorageLight light
) {
	DirectShadingResult res = {};
	uint shadow_index = 0;
	vec3 shadow_coordinates = {};
	uint shadow_type = 0;
	const bool shadow_map = (SHADING_SHADOW_METHOD == SHADING_SHADOW_MAP);
	const bool ray_tracing = (SHADING_SHADOW_METHOD == SHADING_SHADOW_RAY_TRACE);
	if(light.getType() == LightType::Point)
	{
		let pl = PointLight(light);
		vec3 dir = pl.position - sgi.position;
		vec3 wi = Normalize(dir);
		vec3 Le = pl.emission / Length2(dir);

		res.L = material.bsdf<BSDF_FORWARD_BIT>(sgi.geometry_normal, sgi.shading_normal, sgi.out_direction, wi) * Le * AbsDot(sgi.shading_normal, wi);

		
		if(NonZero(res.L))
		{
			if(ray_tracing)
			{
				RayDesc ray;
				ray.Origin = sgi.position;
				ray.Direction = wi;
				ray.TMin = RayTMin(Ray3f(sgi.position, wi), sgi.geometry_normal);
				ray.TMax = length(dir);
				res.L *= scene.testVisibility(ray);
			}
			else if(shadow_map)
			{
				float d = CubeMapDepth(sgi.position, pl.position, pl.z_near);
				//d = 0;
				//d = FloatOffset(d, 1024);
				float v = SceneLightsDepthCube[pl.shadow_texture_index].SampleCmp(LightDepthSampler, -wi, d);
				res.L *= v;
			}

		}
	}
	else if(light.getType() == LightType::Spot)
	{
		let sl = SpotLight(light);
		vec3 dir = sl.position - sgi.position;
		vec3 wi = Normalize(dir);
		vec3 Le = sl.emission / Length2(dir);
	
		const vec4 proj_h = sl.getWorldToProj() * Homogeneous(sgi.position);
		const vec3 proj = HomogeneousNormalize(proj_h);

		const bool in_frustm = proj_h.z > 0 && all(proj.xy >= -1) && all(proj.xy <= 1);
		if(in_frustm)
		{
			Le *= sl.evaluateAttenuation(proj.xy);
			if(NonZero(Le))
			{
				res.L = material.bsdf<BSDF_FORWARD_BIT>(sgi.geometry_normal, sgi.shading_normal, sgi.out_direction, wi) * Le * AbsDot(sgi.shading_normal, wi);
			
				if(NonZero(res.L))
				{
					if(ray_tracing)
					{
						RayDesc ray;
						ray.Origin = sgi.position;
						ray.Direction = wi;
						ray.TMin = RayTMin(Ray3f(sgi.position, wi), sgi.geometry_normal);
						ray.TMax = length(dir);
						res.L *= scene.testVisibility(ray);
					}
					else if(shadow_map)
					{
						float d = proj.z;
						float v = SceneLightsDepth2D[sl.shadow_texture_index].SampleCmp(LightDepthSampler, ClipSpaceToUV(proj.xy), d);
						res.L *= v;
					}
				}
			}
		}
	}
	else if(light.getType() == LightType::Directional)
	{
		let dl = DirectionalLight(light);
		vec3 wi = light.direction;
		vec3 Le = light.emission;
		res.L = material.bsdf<BSDF_FORWARD_BIT>(sgi.geometry_normal, sgi.shading_normal, sgi.out_direction, wi) * Le * AbsDot(sgi.shading_normal, wi);
		if(NonZero(res.L))
		{
			if(ray_tracing)
			{
				RayDesc ray;
				ray.Origin = sgi.position;
				ray.Direction = wi;
				ray.TMin = RayTMin(Ray3f(sgi.position, wi), sgi.geometry_normal);
				ray.TMax = std::numeric_limits<float>::infinity();
				res.L *= scene.testVisibility(ray);
			}
			else if(shadow_map)
			{
				// TODO
			}
		}
	}

	return res;
}

DirectShadingResult Shade(
	const in BoundScene scene,
	const in SurfaceGeometryInfo sgi,
	const in PBMaterial material
) {
	DirectShadingResult res = {};

	const uint n = scene.getUBO().num_lights;
	for(uint i = 0; i < n; ++i)
	{
		res.add(ShadeLight(scene, sgi, material, SceneLights[i]));
	}

	let env_map = BoundScene::EnvMap(scene.getUBO());
	if(NonZero(env_map.solar_disk_emission))
	{
		StorageLight l;
		l.flags = uint(LightType::Directional);
		l.direction = l.position = env_map.solar_disk_direction;
		l.emission = env_map.solar_disk_emission;
		if(env_map.solar_disk_angle > 0)
		{
			l.emission *= sqr(env_map.solar_disk_angle);
		}
		res.add(ShadeLight(scene, sgi, material, l));
	}

	return res;
}