#pragma once

#include "core.glsl"

#ifdef __cplusplus



#else 

// TODO, Make some template-ish types for glsl (maybe we should use a higher level shading language)

#define vec1 float
#define ivec1 int
#define uvec1 uint
#define dvec1 double
#define f16vec1 float16_t
#define hvec1 f16vec1
#define i64vec1 int64_t
#define u64vec1 uint64_t

#define mat1 vec1

#define fvec1 vec1
#define fvec2 vec2
#define fvec3 vec3
#define fvec4 vec4

#define fmat1 mat1
#define fmat2 mat2
#define fmat3 mat3
#define fmat4 mat4

#define Scalar_f float
#define Scalar_i int
#define Scalar_u uint
#define Scalar_d double
#define Scalar_f16 float16_t
#define Scalar_h Scalar_f16
#define Scalar_i64 i64
#define Scalar_u64 u64

// Scalar_id: f, i, u, d, f16, h, i64, u64
#define SCALAR_TYPE(Scalar_id) Scalar_ ## Scalar_id

#define SCALAR_PREFIX_float 
#define SCALAR_PREFIX_int i
#define SCALAR_PREFIX_uint u
#define SCALAR_PREFIX_double d
#define SCALAR_PREFIX_float16_t f16
#define SCALAR_PREFIX_half h
#define SCALAR_PREFIX_int64_t i64
#define SCALAR_PREFIX_uint64_t u64

#define SCALAR_PREFIX(Scalar_typename) SCALAR_PREFIX_ ## Scalar_typename

#define Vector(Dim, Scalar_typename) SCALAR_PREFIX(Scalar_typename) ## vec ## Dim

#define Matrix(Dim, Scalar_typename) SCALAR_PREFIX(Scalar_typename) ## mat ## Dim

#define MatrixX(Cols, Rows, Scalar_typename) SCALAR_PREFIX(Scalar_typename) ## mat ## Cols ## x ## Rows

#endif