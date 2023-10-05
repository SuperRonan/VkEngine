#pragma once
#include <ShaderLib:/common.glsl>

#ifndef N_TYPES_OF_PARTICULES
#error "Forgot to define N_TYPES_OF_PARTICULES"
#endif


#define StorageF    fp16IFP
#define StorageVec2 fp16vec2IFP
#define StorageVec3 fp16vec3IFP
#define StorageVec4 fp16vec4IFP

#ifndef DIMENSIONS
#define DIMENSIONS 2
#endif

#if DIMENSIONS == 2
#define vecD vec2
#elif DIMENSIONS == 3
#define vecD vec3
#endif

#ifndef SYMMETRIC_FORCES
#define SYMMETRIC_FORCES 0
#endif

struct Particule
{
	vecD position;
	uint type;
	vecD velocity;
	float radius;
};

struct ParticuleCommonProperties
{
	StorageVec4 color;
};

struct ForceDescription
{
	StorageVec4 intensity_inv_linear_inv_linear2_contant_linear;
	StorageVec4 intensity_gauss_mu_sigma;
};

float gaussian(float x, float mu, float sigma)
{
	return exp(-0.5 * sqr((x - mu) / sigma)) / (sigma * sqrt(2.0 * 3.1415));
}

vecD modulatePosition(vecD ref, vecD size, vecD p)
{
	const vecD half_size = size * 0.5;
	const vecD diff = p - ref;
	vecD res;
	for(int i=0; i<DIMENSIONS; ++i)
	{
		if(diff[i] > half_size[i])  res[i] = p[i] - size[i];
		else if(diff[i] < -half_size[i]) res[i] = p[i] + size[i];
		else res[i] = p[i];
	}
	return res;
}

struct CommonRuleBuffer
{
	ParticuleCommonProperties particules_properties[N_TYPES_OF_PARTICULES];
	ForceDescription force_descriptions[N_TYPES_OF_PARTICULES * N_TYPES_OF_PARTICULES];
};

uint forceIndex(uint p, uint q)
{
#if SYMMETRIC_FORCES
	const uint pp = p;
	const uint qq = q;
	p = min(pp, qq);
	q = max(pp, qq); 
#endif
	return p * N_TYPES_OF_PARTICULES + q;
}

uint forceIndex(const in Particule p, const in Particule q)
{
	return forceIndex(p.type, q.type);
}

vecD computeForce(const in Particule p, const in Particule q, const in ForceDescription force, vecD world_size)
{
	const vecD pq = modulatePosition(p.position, world_size, q.position) - p.position;
	const float dist2 = dot(pq, pq);
	const float dist = sqrt(dist2);
	const vecD pq_norm = pq / dist;
	const vec4 inv_dist_inv_dist2_constant_linear = vec4(1.0 / dist, 1.0 / dist2, 1.0, dist);
	const float g = gaussian(dist, force.intensity_gauss_mu_sigma.y, force.intensity_gauss_mu_sigma.z);
	const float intensity = dot(vec4(force.intensity_inv_linear_inv_linear2_contant_linear), inv_dist_inv_dist2_constant_linear);// + g * float(force.intensity_gauss_mu_sigma.x);

	const float repultion_radius = (p.radius + q.radius);
	const float repultion_force = 0.05 * min(5e3, (dist < repultion_radius ? (1.0 / sqr(tan(dist2 / repultion_radius * 0.5 * 3.1415))) : 0));

	const vecD res = (pq_norm * (intensity - 0*repultion_force));
	
	return res;
}



struct Uniforms3D
{
	mat4 world_to_camera;
	mat4 camera_to_proj;
	mat4 world_to_proj;
	mat4 proj_to_world;

	vec3 camera_pos;
	vec3 camera_right;
	vec3 camera_up;
	
	vec3 light_dir;
	float roughness;
	vec3 light_color;
	
	vec3 world_size;
	int num_particules;
};