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
	vec3 L;
}

DirectShadingResult Shade(
	const in BoundScene scene,
	const in SurfaceGeometryInfo sgi,
	const in PBMaterial material
) {
	DirectShadingResult res;
	
	// TODO
	res.L = material.albedo;

	return res;
}