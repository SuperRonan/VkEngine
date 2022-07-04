
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
    vec4 intensity_inv_linear_pad;
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