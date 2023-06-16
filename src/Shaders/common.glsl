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

#if SHADER_FP16_AVAILABLE
float16_t sqr(float16_t x)
{
	return x*x;
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

mat2 diagonal(vec2 v)
{
	return mat2(v.x, 0, 0, v.y);
}

mat3 diagonal(vec3 v)
{
	return mat3(
		v.x, 0, 0, 
		0, v.y, 0, 
		0, 0, v.z
	);
}

mat4 diagonal(vec4 v)
{
	return mat4(
		v.x, 0, 0, 0,
		0, v.y, 0, 0, 
		0, 0, v.z, 0,
		0, 0, 0, v.z
	);
}

mat2 scale2(vec2 s)
{
	return diagonal(s);
}

mat3 scale3(vec3 s)
{
	return diagonal(s);
}

mat3 scale3(vec2 s)
{
	return scale3(vec3(s, 1));
}

mat4 scale4(vec4 s)
{
	return diagonal(s);
}

mat4 scale4(vec3 s)
{
	return scale4(vec4(s, 1));
}


mat3 translate3(vec2 t)
{
	mat3 res = mat3(1);
	res[2].xy = t;
	return res;
}

mat4 translate4(vec3 t)
{
	mat4 res = mat4(1);
	res[3].xyz = t;
	return res;
}


mat2 rotation2(float theta)
{
	const float c = cos(theta);
	const float s = sin(theta);
	return mat2(c, -s, s, c);
}

mat3 rotation3X(float theta)
{
	const float c = cos(theta);
	const float s = sin(theta);
	mat3 res = mat3(0);
	res[0].x = 1;
	res[1].yz = vec2(c, -s);
	res[2].yz = vec2(c, s); 
	return res;
}

mat3 rotation3Y(float theta)
{
	const float c = cos(theta);
	const float s = sin(theta);
	mat3 res = mat3(0);
	res[1].y = 1;
	res[0].xz = vec2(c, -s);
	res[2].xz = vec2(c, s); 
	return res;
}

mat3 rotation3Z(float theta)
{
	const float c = cos(theta);
	const float s = sin(theta);
	mat3 res = mat3(0);
	res[2].z = 1;
	res[0].xy = vec2(c, -s);
	res[1].xy = vec2(c, s); 
	return res;
}

mat4 rotation4X(float theta)
{
	return mat4(rotation3X(theta));
}

mat4 rotation4Y(float theta)
{
	return mat4(rotation3Y(theta));
}

mat4 rotation4Z(float theta)
{
	return mat4(rotation3Z(theta));
}

mat3 rotation3XYZ(vec3 xyz)
{
	return rotation3X(xyz.x) * rotation3Y(xyz.y) * rotation3Z(xyz.z);
}

mat4 rotation4XYZ(vec3 xyz)
{
	return mat4(rotation3XYZ(xyz));
}