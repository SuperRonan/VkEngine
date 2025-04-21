#pragma once

#include "core"
#include "bindings"

layout(row_major) uniform;
layout(row_major) buffer;

#define NonUniformEXT(X) nonuniformEXT(X)

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

float sum(vec2 v)
{
	return v.x + v.y;
}

float sum(vec3 v)
{
	return v.x + v.y + v.z;
}

float sum(vec4 v)
{
	return v.x + v.y + v.z + v.w;
}


uint sum(uvec2 v)
{
	return v.x + v.y;
}

uint sum(uvec3 v)
{
	return v.x + v.y + v.z;
}

uint sum(uvec4 v)
{
	return v.x + v.y + v.z + v.w;
}


int sum(ivec2 v)
{
	return v.x + v.y;
}

int sum(ivec3 v)
{
	return v.x + v.y + v.z;
}

int sum(ivec4 v)
{
	return v.x + v.y + v.z + v.w;
}



float sqr(float x)
{
	return x*x;
}

vec2 sqr(vec2 v)
{
	return v*v;
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
	return float16_t(1.0f) / f;
}
#endif

uint dot(uvec2 u, uvec2 v)
{
	uint res = 0;
	for(uint i=0; i < 2; ++i)
	{
		res += u[i] * v[i];
	}
	return res;
}

uint dot(uvec3 u, uvec3 v)
{
	uint res = 0;
	for(uint i=0; i < 3; ++i)
	{
		res += u[i] * v[i];
	}
	return res;
}

uint dot(uvec4 u, uvec4 v)
{
	uint res = 0;
	for(uint i=0; i < 4; ++i)
	{
		res += u[i] * v[i];
	}
	return res;
}

int dot(ivec2 u, ivec2 v)
{
	int res = 0;
	for(uint i=0; i < 2; ++i)
	{
		res += u[i] * v[i];
	}
	return res;
}

int dot(ivec3 u, ivec3 v)
{
	int res = 0;
	for(uint i=0; i < 3; ++i)
	{
		res += u[i] * v[i];
	}
	return res;
}

int dot(ivec4 u, ivec4 v)
{
	int res = 0;
	for(uint i=0; i < 4; ++i)
	{
		res += u[i] * v[i];
	}
	return res;
}

float adot(vec2 u, vec2 v)
{
	return abs(dot(u, v));
}

float adot(vec3 u, vec3 v)
{
	return abs(dot(u, v));
}

float adot(vec4 u, vec4 v)
{
	return abs(dot(u, v));
}

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

uint length2(uvec2 v)
{
	return dot(v, v);
}

uint length2(uvec3 v)
{
	return dot(v, v);
}

uint length2(uvec4 v)
{
	return dot(v, v);
}

uint length2(ivec2 v)
{
	return dot(v, v);
}

uint length2(ivec3 v)
{
	return dot(v, v);
}

uint length2(ivec4 v)
{
	return dot(v, v);
}

float length(uvec2 v)
{
	return sqrt(float(length2(v)));
}

float length(uvec3 v)
{
	return sqrt(float(length2(v)));
}

float length(uvec4 v)
{
	return sqrt(float(length2(v)));
}

float length(ivec2 v)
{
	return sqrt(float(length2(v)));
}

float length(ivec3 v)
{
	return sqrt(float(length2(v)));
}

float length(ivec4 v)
{
	return sqrt(float(length2(v)));
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

vec2 safeNormalize(vec2 v)
{
	return length2(v) == 0 ? v : normalize(v);
}

vec3 safeNormalize(vec3 v)
{
	return length2(v) == 0 ? v : normalize(v);
}

vec4 safeNormalize(vec4 v)
{
	return length2(v) == 0 ? v : normalize(v);
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

vec3 triangleUVToBarycentric(vec2 uv)
{
	return vec3(1.0f - uv.x - uv.y, uv.x, uv.y);
}

float floatOffset(float f, int o)
{
	return intBitsToFloat(floatBitsToInt(f) + o);
}

float quantize(float x, float q)
{
	return x - mod(x, q);
}

vec2 quantize(vec2 x, float q)
{
	return x - mod(x, q);
}

vec3 quantize(vec3 x, float q)
{
	return x - mod(x, q);
}

vec4 quantize(vec4 x, float q)
{
	return x - mod(x, q);
}

ivec3 cross(ivec3 a, ivec3 b)
{
	// return ivec3(
	// 	a.y * b.z - a.z * b.y,
	// 	a.z * b.x - a.x * b.z,
	// 	a.x * b.y - a.y * b.x
	// );
	return a.yzx * b.zxy - a.zxy * b.yzx; 
}

uvec3 cross(uvec3 a, uvec3 b)
{
	// return uvec3(
	// 	a.y * b.z - a.z * b.y,
	// 	a.z * b.x - a.x * b.z,
	// 	a.x * b.y - a.y * b.x
	// );
	return a.yzx * b.zxy - a.zxy * b.yzx; 
}


float sinc(float x)
{
	return sin(x) / x;
}

vec2 sinc(vec2 x)
{
	return sin(x) / x;
}

vec3 sinc(vec3 x)
{
	return sin(x) / x;
}

vec4 sinc(vec4 x)
{
	return sin(x) / x;
}

float sinch(float x)
{
	return sinh(x) / x;
}

vec2 sinch(vec2 x)
{
	return sinh(x) / x;
}

vec3 sinch(vec3 x)
{
	return sinh(x) / x;
}

vec4 sinch(vec4 x)
{
	return sinh(x) / x;
}

float Luminance(vec3 rgb)
{
	return dot(rgb, vec3(0.299, 0.587, 0.114));
}

float Grey(vec3 rgb)
{
	return sum(rgb) / 3;
}

bool isWrong(float f)
{
	return isnan(f) || isinf(f);
}

bvec2 isWrong(vec2 v)
{
	bvec2 res;
	for(uint i = 0; i < 2; ++i)
	{
		res[i] = isWrong(v[i]);
	}
	return res;
}

bvec3 isWrong(vec3 v)
{
	bvec3 res;
	for(uint i = 0; i < 3; ++i)
	{
		res[i] = isWrong(v[i]);
	}
	return res;
}

bvec4 isWrong(vec4 v)
{
	bvec4 res;
	for(uint i = 0; i < 4; ++i)
	{
		res[i] = isWrong(v[i]);
	}
	return res;
}

float fixExtremeToZero(float f)
{
	if(isWrong(f))
	{
		f = 0;
	}
	return f;
}

vec2 fixExtremeToZero(vec2 v)
{
	for(uint i = 0; i < 2; ++i)
	{
		v[i] = fixExtremeToZero(v[i]);
	}
	return v;
}

vec3 fixExtremeToZero(vec3 v)
{
	for(uint i = 0; i < 3; ++i)
	{
		v[i] = fixExtremeToZero(v[i]);
	}
	return v;
}

vec4 fixExtremeToZero(vec4 v)
{
	for(uint i = 0; i < 4; ++i)
	{
		v[i] = fixExtremeToZero(v[i]);
	}
	return v;
}

bool nonZero(vec3 rgb)
{
	return any(notEqual(rgb, 0..xxx));
}

#include "constants"
