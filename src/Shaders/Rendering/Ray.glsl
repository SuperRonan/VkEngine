
#include <ShaderLib:/common.glsl>

struct Ray
{
    vec3 origin;
    vec3 direction;
};

vec3 sampleRay(const in Ray r, float t)
{
    return r.origin + t * r.direction;
}