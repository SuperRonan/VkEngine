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
		Light::Sample sample = light.sampleLi(rng, sgi.position, sgi.shading_normal, sgi.out_direction);
		const vec3 bsdf = material.bsdf(sgi.geometry_normal, sgi.shading_normal, sgi.out_direction, sample.direction);
		res = max(sample.intensity, vec3(0)) * bsdf * abs(dot(sgi.shading_normal, sample.direction));
		if(NonZero(res))
		{
			RayDesc ray;
			ray.Origin = sgi.position;
			ray.Direction = sample.direction;
			ray.TMin = RayTMin(ray.getRay(), sgi.geometry_normal);
			ray.TMax = sample.distance;
			const bool v = scene.testVisibility(ray);
			if(!v)
			{
				res = vec3(0);
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
			ILight l;
			if(MakeLight(SceneLights[i], l))
			{
				vec3 light_contrib = vec3(0);
				for(uint s = 0; s < sample_per_light; ++s)
				{
					light_contrib += FitWrongToZero(shadeLight(sgi, material, l, rng));
				}
				res += (light_contrib / float(sample_per_light));
			}
		}
		return res;
	}


};

