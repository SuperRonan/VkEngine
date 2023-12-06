#pragma once


#ifndef COMMON_DESCRIPTOR_BINDING
#define COMMON_DESCRIPTOR_BINDING set = 0, binding = 0
#endif

#ifndef SCENE_DESCRIPTOR_BINDING
#define SCENE_DESCRIPTOR_BINDING set = 1, binding = 0
#endif

#ifndef MODULE_DESCRIPTOR_BINDING
#define MODULE_DESCRIPTOR_BINDING set = 2, binding = 0
#endif

#ifndef SHADER_DESCRIPTOR_BINDING
#define SHADER_DESCRIPTOR_BINDING set = 3, binding = 0
#endif

#ifndef INVOCATION_DESCRIPTOR_BINDING
#define INVOCATION_DESCRIPTOR_BINDING set = 4, binding = 0
#endif

#extension GL_EXT_shader_explicit_arithmetic_types : require

#if SHADER_FP16_AVAILABLE

#define fp16IFP float16_t
#define fp16vec2IFP f16vec2 
#define fp16vec3IFP f16vec3 
#define fp16vec4IFP f16vec4 

#else

#define fp16IFP float
#define fp16vec2IFP vec2 
#define fp16vec3IFP vec3 
#define fp16vec4IFP vec4 

#endif

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int2 ivec2
#define int3 ivec3
#define int4 ivec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define lerp mix



float sqr(float x)
{
	return x*x;
}

float rcp(float f)
{
	return 1.0f / f;
}

vec2 rcp(vec2 v)
{
	return 1.0f / v;
}
vec3 rcp(vec3 v)
{
	return 1.0f / v;
}
vec4 rcp(vec4 v)
{
	return 1.0f / v;
}


#if SHADER_FP16_AVAILABLE
float16_t sqr(float16_t x)
{
	return x*x;
}

float16_t rcp(float16_t f)
{
	return 1.0f / f;
}
#endif

float length2(vec2 v)
{
	return dot(v, v);
}

float length2(vec3 v)
{
	return dot(v, v);
}

float length2(vec4 v)
{
	return dot(v, v);
}

float distance2(vec2 a, vec2 b)
{
	const vec2 d = b - a;
	return length2(d);
}

float distance2(vec3 a, vec3 b)
{
	const vec3 d = b - a;
	return length2(d);
}

float distance2(vec4 a, vec4 b)
{
	const vec4 d = b - a;
	return length2(d);
}


vec2 clipSpaceToUV(vec2 cp)
{
	return (cp * 0.5f) + 0.5f;
}

vec2 UVToClipSpace(vec2 uv)
{
	return (uv * 2.0f) - 1.0f;
}

float pixelToU(int p, int d)
{
	return (float(p) + 0.5f) / float(d);
}

float pixelToU(uint p, uint d)
{
	return (float(p) + 0.5f) / float(d);
}

vec2 pixelToUV(ivec2 pix, ivec2 dims)
{
	return (vec2(pix) + 0.5f) / vec2(dims);
}

vec2 pixelToUV(uvec2 pix, uvec2 dims)
{
	return (vec2(pix) + 0.5f) / vec2(dims);
}

vec3 pixelToUVW(ivec3 pix, ivec3 dims)
{
	return (vec3(pix) + 0.5f) / vec3(dims);
}

vec3 pixelToUVW(uvec3 pix, uvec3 dims)
{
	return (vec3(pix) + 0.5f) / vec3(dims);
}

// Generalized cross product matrix
mat2 tilt(vec2 a, vec2 b)
{
	return outerProduct(a, b) - outerProduct(b, a);
}

mat3 tilt(vec3 a, vec3 b)
{
	return outerProduct(a, b) - outerProduct(b, a);
}

mat4 tilt(vec4 a, vec4 b)
{
	return outerProduct(a, b) - outerProduct(b, a);
}


#define PI 3.1415926535897932384626433832795
#define HALF_PI (PI / 2.0)
#define QUART_PI (PI / 4.0)
#define TWO_PI (2.0 * PI)
#define oo_PI rcp(PI)
#define EULER_e 2.7182818284590452353602874713527
#define SQRT_2 (sqrt(2.0))
#define oo_SQRT_2 rcp(SQRT_2)
#define GOLDEN_RATIO ((1.0 + sqrt(5.0)) / 2.0)
#define PHI GOLDEN_RATIO
#define EPSILON_f 1.19209e-07f
#define EPSILON_d double(2.22045e-16)
#define EPSILON_h float16_t(1e-3) // TODO
#define EPSILON EPSILON_f