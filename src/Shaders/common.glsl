#pragma once


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

vec2 clipSpaceToUV(vec2 cp)
{
	return (cp * 0.5f) + 0.5f;
}

vec2 UVToClipSpace(vec2 uv)
{
	return (uv * 2.0f) - 1.0f;
}
