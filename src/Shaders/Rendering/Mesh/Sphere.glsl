#include <ShaderLib:/common.glsl>

#include "../Ray.glsl"

struct Sphere
{
    vec3 center;
    float radius;
};

struct RaySphereIntersection
{
    vec3 point;
    float t;
};

