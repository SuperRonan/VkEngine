#pragma once
#include "../Shaders/common.glsl"

#ifndef N_TYPES_OF_PARTICULES
#error "Forgot to define N_TYPES_OF_PARTICULES"
#endif


#define StorageF    fp16IFP
#define StorageVec2 fp16vec2IFP
#define StorageVec3 fp16vec3IFP
#define StorageVec4 fp16vec4IFP

struct Particule
{
    vec2 position;
    vec2 velocity;
    uint type;
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

vec2 modulatePosition(vec2 ref, vec2 size, vec2 p)
{
    const vec2 half_size = size * 0.5;
    const vec2 diff = p - ref;
    vec2 res;
    for(int i=0; i<2; ++i)
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
    return p * N_TYPES_OF_PARTICULES + q;
}

uint forceIndex(const in Particule p, const in Particule q)
{
    return forceIndex(p.type, q.type);
}

vec2 computeForce(const in Particule p, const in Particule q, const in ForceDescription force, vec2 world_size)
{
    const vec2 pq = modulatePosition(p.position, world_size, q.position) - p.position;
    const float dist2 = dot(pq, pq);
    const float dist = sqrt(dist2);
    const vec2 pq_norm = pq / dist;
    const vec4 inv_dist_inv_dist2_constant_linear = vec4(1.0 / dist, 1.0 / dist2, 1.0, dist);
    const float g = gaussian(dist, force.intensity_gauss_mu_sigma.y, force.intensity_gauss_mu_sigma.z);
    const float intensity = dot(vec4(force.intensity_inv_linear_inv_linear2_contant_linear), inv_dist_inv_dist2_constant_linear) + g * float(force.intensity_gauss_mu_sigma.x);
    
    const float repultion_radius = (p.radius + q.radius);
    const float repution_force = min(5e3, (dist < repultion_radius ? (1.0 / sqr(tan(dist2 / repultion_radius * 0.5 * 3.1415))) : 0));
    const vec2 repultion = -repution_force * pq_norm * 0.05;

    const vec2 res = pq_norm * intensity + repultion;
    return res;
}