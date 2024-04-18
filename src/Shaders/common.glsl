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
#define POSITIVE_INF_f (1.0f / 0.0f)
#define NEGATIVE_INF_f (-1.0f / 0.f)