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

// Returns ray dist to intersection
float intersectOutside(const in Sphere sphere, const in Ray ray)
{
    const vec3 sphere_to_ray = ray.origin - sphere.center;
    const float a = 1;
    const float b = dot(sphere_to_ray, ray.direction) * 2.0f;
    const float c = dot(sphere_to_ray, sphere_to_ray) - sqr(sphere.radius); 
    const float delta = b * b - 4.0f * a * c;
    
    float res;
    if(abs(delta) < EPSILON)
    {
        res = (-b) / (2.0f * a);
    }
    else if(delta > 0)
    {
        const float left = (-b) / (2.0f * a);
        const float right = sqrt(delta) / (2.0f * a); 
        res = left - right;
    }
    else
    {
        res = -1.0f;
    }



    return res;
}