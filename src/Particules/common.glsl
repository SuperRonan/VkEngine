
struct Particule
{
    vec2 position;
    vec2 velocity;
    uint type;
    int pad;
};

struct ParticuleCommonProperties
{
    vec4 color;
};

struct ForceDescription
{
    vec4 intensity_inv_linear_inv_linear2_contant_linear;
};

#ifndef N_TYPES_OF_PARTICULES
#define N_TYPES_OF_PARTICULES 2
#endif

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

vec2 computeForce(const in Particule p, const in Particule q, const in ForceDescription force)
{
    const vec2 pq = q.position - p.position;
    const float dist2 = dot(pq, pq);
    const float dist = sqrt(dist2);
    const vec2 pq_norm = pq / dist;
    const vec4 inv_dist_inv_dist2_constant_linear = vec4(1.0 / dist, 1.0 / dist2, 1.0, dist);
    const float intensity = dot(force.intensity_inv_linear_inv_linear2_contant_linear, inv_dist_inv_dist2_constant_linear);
    const vec2 res = pq_norm * intensity;
    return res;
}