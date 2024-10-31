#pragma once

#include <ShaderLib:/interop_glsl_cpp>

#include "common.glsl"

float TanHalfFOVFast(float fov)
{
	// See Graphics Gems VIII.5
	const float x = fov;
	const float x3 = x * sqr(x);
	const float x5 = x3 * sqr(x);
	const float res = (x + rcp(12) * x3 + rcp(120) * x5) * 0.5f;
	return res;
}

float TanHalfFOVCorrect(float fov)
{
	const float res = tan(fov * 0.5f);
	return res;
}

float TanHalfFOV(float fov)
{
	return TanHalfFOVCorrect(fov);
}

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

mat2 DiagonalMatrix(vec2 v)
{
	return mat2(v.x, 0, 0, v.y);
}

mat3 DiagonalMatrix(vec3 v)
{
	return mat3(
		v.x, 0, 0, 
		0, v.y, 0, 
		0, 0, v.z
	);
}

mat4 DiagonalMatrix(vec4 v)
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

mat2 Scale2(vec2 s)
{
	return DiagonalMatrix(s);
}

mat3 Scale3(vec3 s)
{
	return DiagonalMatrix(s);
}

mat3 Scale3(vec2 s)
{
	return Scale3(vec3(s, 1));
}

mat4 Scale4(vec4 s)
{
	return DiagonalMatrix(s);
}

mat4 Scale4(vec3 s)
{
	return Scale4(vec4(s, 1));
}


mat3 Translate3(vec2 t)
{
	mat3 res = mat3(1);
	res[2].xy = t;
	return res;
}

mat4 Translate4(vec3 t)
{
	mat4 res = mat4(1);
	res[3].xyz = t;
	return res;
}


mat2 Rotation2(float theta)
{
	const float c = cos(theta);
	const float s = sin(theta);
	return mat2(c, -s, s, c);
}

mat3 Rotation3X(float theta)
{
	const float c = cos(theta);
	const float s = sin(theta);
	mat3 res = mat3(0);
	res[0].x = 1;
	res[1].yz = vec2(c, -s);
	res[2].yz = vec2(c, s); 
	return res;
}

mat3 Rotation3Y(float theta)
{
	const float c = cos(theta);
	const float s = sin(theta);
	mat3 res = mat3(0);
	res[1].y = 1;
	res[0].xz = vec2(c, -s);
	res[2].xz = vec2(c, s); 
	return res;
}

mat3 Rotation3Z(float theta)
{
	const float c = cos(theta);
	const float s = sin(theta);
	mat3 res = mat3(0);
	res[2].z = 1;
	res[0].xy = vec2(c, -s);
	res[1].xy = vec2(c, s); 
	return res;
}

mat4 Rotation4X(float theta)
{
	return mat4(Rotation3X(theta));
}

mat4 Rotation4Y(float theta)
{
	return mat4(Rotation3Y(theta));
}

mat4 Rotation4Z(float theta)
{
	return mat4(Rotation3Z(theta));
}

mat3 Rotation3XYZ(vec3 xyz)
{
	return Rotation3X(xyz.x) * Rotation3Y(xyz.y) * Rotation3Z(xyz.z);
}

mat4 Rotation4XYZ(vec3 xyz)
{
	return mat4(Rotation3XYZ(xyz));
}

mat3 DirectionMatrix(const in mat3 xform)
{
	return transpose(inverse(xform));
}

mat4x3 MakeAffineTransform(const in mat3 Q, const in vec3 T);

// Matrices follow the vulkan standard
// - left handed 
// - depth zero to one 

// LookAt matrix is a Rigid matrix
// front, up and right are assumed to make an orthonormal basis
mat4x3 LookAtDir4x3AssumeOrtho(vec3 position, vec3 front, vec3 up, vec3 right)
{
	const mat3 R = transpose(mat3(right, up, front));
	const vec3 T = -R * position;
	const mat4x3 res = MakeAffineTransform(R, T);
	return res;
}

// front and up are assumed to be normalized directions
// dot(up, front) == 0
mat4x3 LookAtDir4x3AssumeOrtho(vec3 position, vec3 front, vec3 up)
{	
	const vec3 f = front;
	const vec3 s = cross(up, front);
	const vec3 u = -up;
	return LookAtDir4x3AssumeOrtho(position, front, u, s);
}

mat4x3 LookAtDir4x3(vec3 position, vec3 front, vec3 up)
{
	const vec3 ss = cross(up, front);
	const float sinus = length(ss);
	const vec3 s = ss / sinus;
	const vec3 u = -(up - front * dot(front, up)) / sinus;
	// const vec3 s = normalize(cross(up, front));
	// const vec3 u = cross(s, front);
	return LookAtDir4x3AssumeOrtho(position, front, u, s);
}

// up is assumed to a normalized direction
mat4x3 LookAt4x3(vec3 position, vec3 center, vec3 up)
{
	return LookAtDir4x3(position, normalize(center - position), up);
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
	if(abs(dot(o, Z)) >= l)
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
#define AFFINE_XFORM_INL_Dims 2
#define AFFINE_XFORM_INL_Scalar float
#define AFFINE_XFORM_INL_QBlock mat2
#define AFFINE_XFORM_INL_XFormMatrix mat3x2
#define AFFINE_XFORM_INL_FullMatrix mat3
#define AFFINE_XFORM_INL_Vector vec2

#include "TemplateImpl/AffineXForm.inl"

#undef AFFINE_XFORM_INL_TEMPLATE_DECL 
#undef AFFINE_XFORM_INL_Dims 
#undef AFFINE_XFORM_INL_Scalar 
#undef AFFINE_XFORM_INL_QBlock 
#undef AFFINE_XFORM_INL_XFormMatrix 
#undef AFFINE_XFORM_INL_FullMatrix 
#undef AFFINE_XFORM_INL_Vector 


#define AFFINE_XFORM_INL_TEMPLATE_DECL 
#define AFFINE_XFORM_INL_Dims 3
#define AFFINE_XFORM_INL_Scalar float
#define AFFINE_XFORM_INL_QBlock mat3
#define AFFINE_XFORM_INL_XFormMatrix mat4x3
#define AFFINE_XFORM_INL_FullMatrix mat4
#define AFFINE_XFORM_INL_Vector vec3

#include "TemplateImpl/AffineXForm.inl"

#undef AFFINE_XFORM_INL_TEMPLATE_DECL 
#undef AFFINE_XFORM_INL_Dims 
#undef AFFINE_XFORM_INL_Scalar 
#undef AFFINE_XFORM_INL_QBlock 
#undef AFFINE_XFORM_INL_XFormMatrix 
#undef AFFINE_XFORM_INL_FullMatrix 
#undef AFFINE_XFORM_INL_Vector 


#define CLIP_SPACE_INL_TEMPLATE_DECL
#define CLIP_SPACE_INL_Scalar float
#define CLIP_SPACE_INL_Vector(N) vec ## N
#define CLIP_SPACE_INL_Matrix(N) mat ## N

#include "TemplateImpl/ClipSpaceMatrices.inl"

#undef CLIP_SPACE_INL_TEMPLATE_DECL
#undef CLIP_SPACE_INL_Scalar
#undef CLIP_SPACE_INL_Vector
#undef CLIP_SPACE_INL_Matrix