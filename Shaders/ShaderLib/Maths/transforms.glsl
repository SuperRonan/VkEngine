#pragma once

#include "common.glsl"

// theta is the inclination angle with the +Y axis
// phi is the azimuthal angle with the +X axis

vec3 SphericalToCartesian(vec2 theta_phi)
{
	const float theta = theta_phi.x;
	const float phi = theta_phi.y;
	const float ct = cos(theta);
	const float st = sin(theta);
	const float cp = cos(phi);
	const float sp = sin(phi);
	vec3 res;
	res.x = st * cp;
	res.y = ct;
	res.z = st * sp;
	return res;
}

vec3 SphericalToCartesian(vec3 theta_phi_rho)
{
	return SphericalToCartesian(theta_phi_rho.xy) * theta_phi_rho.z;
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

vec2 ExtractDiagonal(mat2 m)
{
	return vec2(m[0][0], m[1][1]);
}

vec3 ExtractDiagonal(mat3 m)
{
	return vec3(m[0][0], m[1][1], m[2][2]);
}

vec4 ExtractDiagonal(mat4 m)
{
	return vec4(m[0][0], m[1][1], m[2][2], m[3][3]);
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

// front and up are assumed to be normalized directions
mat4x3 LookAtDir4x3(vec3 position, vec3 front, vec3 up)
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
	res[3][0] =-dot(s, position);
	res[3][1] =-dot(u, position);
	res[3][2] =-dot(f, position);
	return res;
}

// up is assumed to a normalized direction
mat4x3 LookAt4x3(vec3 position, vec3 center, vec3 up)
{
	return LookAtDir4x3(position, normalize(position + center), up);
}

mat4 InfinitePerspectiveProjFromTan(float tan_half_fov, float aspect, float z_near)
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

mat4 InfinitePerspectiveProjFromFOV(float fov, float aspect, float z_near)
{
	const float tan_half_fov = tan(fov * 0.5);
	return InfinitePerspectiveProjFromTan(tan_half_fov, aspect, z_near);
}

// Assume dir is normalized
mat3 BasisFromDir(vec3 dir)
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

vec3 ComputeTriangleNormal(const in mat3 vertices)
{
	return normalize(cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
}


#define AFFINE_XFORM_INL_TEMPLATE_DECL 
#define AFFINE_XFORM_INL_cpp_constexpr 
#define AFFINE_XFORM_INL_Dims 2
#define AFFINE_XFORM_INL_CRef(T) const in T
#define AFFINE_XFORM_INL_nmspc 
#define AFFINE_XFORM_INL_Scalar float
#define AFFINE_XFORM_INL_QBlock mat2
#define AFFINE_XFORM_INL_XFormMatrix mat3x2
#define AFFINE_XFORM_INL_FullMatrix mat3
#define AFFINE_XFORM_INL_Vector vec2

#include "AffineXForm.inl"

#undef AFFINE_XFORM_INL_TEMPLATE_DECL 
#undef AFFINE_XFORM_INL_cpp_constexpr 
#undef AFFINE_XFORM_INL_Dims 
#undef AFFINE_XFORM_INL_CRef
#undef AFFINE_XFORM_INL_nmspc 
#undef AFFINE_XFORM_INL_Scalar 
#undef AFFINE_XFORM_INL_QBlock 
#undef AFFINE_XFORM_INL_XFormMatrix 
#undef AFFINE_XFORM_INL_FullMatrix 
#undef AFFINE_XFORM_INL_Vector 


#define AFFINE_XFORM_INL_TEMPLATE_DECL 
#define AFFINE_XFORM_INL_cpp_constexpr 
#define AFFINE_XFORM_INL_Dims 3
#define AFFINE_XFORM_INL_CRef(T) const in T
#define AFFINE_XFORM_INL_nmspc 
#define AFFINE_XFORM_INL_Scalar float
#define AFFINE_XFORM_INL_QBlock mat3
#define AFFINE_XFORM_INL_XFormMatrix mat4x3
#define AFFINE_XFORM_INL_FullMatrix mat4
#define AFFINE_XFORM_INL_Vector vec3

#include "AffineXForm.inl"

#undef AFFINE_XFORM_INL_TEMPLATE_DECL 
#undef AFFINE_XFORM_INL_cpp_constexpr 
#undef AFFINE_XFORM_INL_Dims 
#undef AFFINE_XFORM_INL_CRef
#undef AFFINE_XFORM_INL_nmspc 
#undef AFFINE_XFORM_INL_Scalar 
#undef AFFINE_XFORM_INL_QBlock 
#undef AFFINE_XFORM_INL_XFormMatrix 
#undef AFFINE_XFORM_INL_FullMatrix 
#undef AFFINE_XFORM_INL_Vector 
