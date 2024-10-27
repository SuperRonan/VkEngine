#pragma once

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

Ray TransformAssumeRigid(const in mat4x3 xform, Ray ray)
{
    ray.origin = xform * vec4(ray.origin, 1);
    ray.direction = mat3(xform) * ray.direction;
    return ray;
}


