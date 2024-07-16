#pragma once

#include "common.glsl"

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

mat3 directionMatrix(const in mat3 xform)
{
	return transpose(inverse(xform));
}


// Matrices follow the vulkan standard
// - left handed 
// - depth zero to one 

// Copy pasted from glm

mat4x3 lookAtAssumeNormalized4x3(vec3 center, vec3 front, vec3 up)
{	
	const vec3 f = front;
	const vec3 s = normalize(cross(up, front));
	const vec3 u = normalize(cross(f, s));
	
	mat4x3 res = mat4x3(1);
	res[0][0] = s.x;
	res[1][0] = s.y;
	res[2][0] = s.z;
	res[0][1] = u.x;
	res[1][1] = u.y;
	res[2][1] = u.z;
	res[0][2] = f.x;
	res[1][2] = f.y;
	res[2][2] = f.z;
	res[3][0] =-dot(s, center);
	res[3][1] =-dot(u, center);
	res[3][2] =-dot(f, center);
	return res;
}

mat4 infinitePerspectiveProjFromTan(float tan_half_fov, float aspect, float z_near)
{
	const float range = tan_half_fov * z_near;
	const float left = -range * aspect;
	const float right = range * aspect;
	const float bottom = -range;
	const float top = range;

	mat4 res = mat4(0);
	res[0][0] = (2 * z_near) / (right - left);
	res[1][1] = - (2 * z_near) / (top - bottom);
	res[2][2] = 1;
	res[2][3] = 1;
	res[3][2] = - 2 * z_near;
	return res;
}

mat4 infinitePerspectiveProjFromFOV(float fov, float aspect, float z_near)
{
	const float tan_half_fov = tan(fov * 0.5);
	return infinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near);
}

// Assume dir is normalized
mat3 basisFromDir(vec3 dir)
{
	const vec3 Z = dir;
	mat3 res;
	res[2] = Z;
	vec3 o;
	o = vec3(1, 0, 0);
	const float l = 0.5;
	if(dot(o, Z) >= l)
	{
		o = vec3(0, 1, 0);
		// if(dot(o, Z) >= l)
		// {
		// 	o = vec3(0, 0, 1);
		// }
	}
	const vec3 X = normalize(cross(o, Z));
	const vec3 Y = cross(X, Z);
	res[0] = X;
	res[1] = Y;
	return res;
}


mat2x4 homogenizeAsVectorArray(const in mat2x3 m, float h)
{
	mat2x4 res;
	for(uint i=0; i <2; ++i)
	{
		res[i] = vec4(m[i], h);
	}
	return res;
}

mat3x4 homogenizeAsVectorArray(const in mat3x3 m, float h)
{
	mat3x4 res;
	for(uint i=0; i <3; ++i)
	{
		res[i] = vec4(m[i], h);
	}
	return res;
}

mat4x4 homogenizeAsVectorArray(const in mat4x3 m, float h)
{
	mat4x4 res;
	for(uint i=0; i <4; ++i)
	{
		res[i] = vec4(m[i], h);
	}
	return res;
}

vec3 computeTriangleNormal(const in mat3 m)
{
	return normalize(cross(m[1] - m[0], m[2] - m[0]));
}