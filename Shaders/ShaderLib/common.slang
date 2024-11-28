#pragma once

// https://github.com/shader-slang/slang/issues/5303
//#pragma warning(disable: 15205)

#include "core"
#include "bindings"

#include "interop_slang_cpp"

typealias i8  = int8_t;
typealias i16 = int16_t;
typealias i32 = int32_t;
typealias i64 = int64_t;

typealias u8  = uint8_t;
typealias u16 = uint16_t;
typealias u32 = uint32_t;
typealias u64 = uint64_t;

typealias f16 = float16_t;
typealias f32 = float32_t;
typealias f64 = float64_t;


#if SHADER_FP16_AVAILABLE
// typedef half fp16IFP
typealias fp16IFP = half;
#else
typealias fp16IFP = float;
#endif

template <typename T, uint N>
typealias Vector = vector<T, N>;

template <typename T, uint R, uint C>
typealias Matrix = matrix<T, R, C>;

template <typename T, uint R, uint C>
Vector<T, R> GetColumn(Matrix<T, R, C> m, uint i)
{
	Vector<T, R> res;
	for(uint j = 0; j < R; ++j)
	{
		res[j] = m[j][i];
	}
	return res;
}

template <typename T, uint R, uint C>
Vector<T, C> GetRow(Matrix<T, R, C> m, uint i)
{
	return m[i];
}

template <typename T, uint R, uint C>
void SetColumn(inout Matrix<T, R, C> m, uint i, Vector<T, R> v)
{
	for(uint j = 0; j < R; ++j)
	{
		m[j][i] = v[j];
	}
}

template <typename T, uint R, uint C>
void SetRow(inout Matrix<T, R, C> m, uint i, Vector<T, C> v)
{
	m[i] = v;
}

template <uint N>
typealias vec = vector<float, N>;
typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;

template <uint N>
typealias ivec = vector<int, N>;
typealias ivec2 = int2;
typealias ivec3 = int3;
typealias ivec4 = int4;

template <uint N>
typealias uvec = vector<uint, N>;
typealias uvec2 = uint2;
typealias uvec3 = uint3;
typealias uvec4 = uint4;

template <uint N>
typealias bvec = vector<bool, N>;
typealias bvec2 = bool2;
typealias bvec3 = bool3;
typealias bvec4 = bool4;

template <uint N>
typealias dvec = vector<bool, N>;
typealias dvec2 = double2;
typealias dvec3 = double3;
typealias dvec4 = double4;

template <uint N>
typealias hvec = vector<half, N>;
typealias hvec2 = half2;
typealias hvec3 = half3;
typealias hvec4 = half4;


typealias mat2 = float2x2;
typealias mat2x2 = mat2;
typealias mat3 = float3x3;
typealias mat3x3 = mat3;
typealias mat4 = float4x4;
typealias mat4x4 = mat4;

typealias dmat2 = double2x2;
typealias dmat2x2 = dmat2;
typealias dmat3 = double3x3;
typealias dmat3x3 = dmat3;
typealias dmat4 = double4x4;
typealias dmat4x4 = dmat4;

// Slang's matrix<T, Rows, Columns, L> (like floatRxC) follows the mathematical convension, 
// and is stored in row major by default
// m[i] returns the i-th row of the matrix

// GLSL's matCxR follows the oposite convension,
// and is stored in column major by default
// And a GLSL's mat[i] returns the i-th column
template <uint Colums, uint Rows>
typealias mat = matrix<float, Colums, Rows>;

typealias mat2x3 = matrix<float, 3, 2>;
typealias mat2x4 = matrix<float, 4, 2>;

typealias mat3x2 = matrix<float, 2, 3>;
typealias mat3x4 = matrix<float, 4, 3>;

typealias mat4x2 = matrix<float, 2, 4>;
typealias mat4x3 = matrix<float, 4, 4>;


typealias dmat2x3 = matrix<double, 3, 2>;
typealias dmat2x4 = matrix<double, 4, 2>;

typealias dmat3x2 = matrix<double, 2, 3>;
typealias dmat3x4 = matrix<double, 4, 3>;

typealias dmat4x2 = matrix<double, 2, 4>;
typealias dmat4x3 = matrix<double, 4, 4>;


typealias fp16vecIFP<let N : uint> = vector<fp16IFP, N>;
typealias fp16vec2IFP = fp16vecIFP<2>;
typealias fp16vec3IFP = fp16vecIFP<3>;
typealias fp16vec4IFP = fp16vecIFP<4>;

#define mix lerp

#define FOR_EACH_BUILTIN_FLOAT(Macro) \
Macro(f16) \
Macro(f32) \
Macro(f64)

#define FOR_EACH_BUILTIN_INT_TYPE(Macro) \
Macro(i8) \
Macro(i16) \
Macro(i32) \
Macro(i64)

#define FOR_EACH_BUILTIN_UINT_TYPE(Macro) \
Macro(u8) \
Macro(u16) \
Macro(u32) \
Macro(u64)

#define FOR_EACH_BUILTIN_INTEGRAL(Macro) FOR_EACH_BUILTIN_INT_TYPE(Macro) FOR_EACH_BUILTIN_UINT_TYPE(Macro)

// Does not include bool
#define FOR_EACH_BUILTIN_TYPE(Macro) FOR_EACH_BUILTIN_INTEGRAL(Macro) FOR_EACH_BUILTIN_FLOAT(Macro)

#define CONVERT_BUILTIN_CASE(test_type) \
	else if(From is test_type)	{res = Target((i as test_type).value);}

#define DECLARE_CONVERT_BUILTIN_To(Dst) \
template <typename From : __BuiltinType> \
Dst ConvertBuiltInTo_ ## Dst(From i) \
{ \
	Dst res; \
	typealias Target = Dst; \
	if(false){} \
	FOR_EACH_BUILTIN_TYPE(CONVERT_BUILTIN_CASE) \
	return res; \
}

// Sadly, we can't do this, because FOR_EACH_BUILTIN_TYPE is referenced in DECLARE_CONVERT_BUILTIN_To, and it is considered self-referencial and thus not expanded
//FOR_EACH_BUILTIN_TYPE(DECLARE_CONVERT_BUILTIN_To)

namespace impl
{	
	DECLARE_CONVERT_BUILTIN_To(f16)
	DECLARE_CONVERT_BUILTIN_To(f32)
	DECLARE_CONVERT_BUILTIN_To(f64)
	
	DECLARE_CONVERT_BUILTIN_To(i8)
	DECLARE_CONVERT_BUILTIN_To(i16)
	DECLARE_CONVERT_BUILTIN_To(i32)
	DECLARE_CONVERT_BUILTIN_To(i64)
	
	DECLARE_CONVERT_BUILTIN_To(u8)
	DECLARE_CONVERT_BUILTIN_To(u16)
	DECLARE_CONVERT_BUILTIN_To(u32)
	DECLARE_CONVERT_BUILTIN_To(u64)
}

#define CONVERT_BUILTIN_CASE_2(test_type) \
	else if(Target is test_type){res = reinterpret<Target>(impl :: ConvertBuiltInTo_ ## test_type (i));}

template <typename Target : __BuiltinType, typename From : __BuiltinType>
Target ConvertBuiltInTo(From i)
{
	Target res;
	if(false){}
	FOR_EACH_BUILTIN_TYPE(CONVERT_BUILTIN_CASE_2)
	return res;
}

template <typename Target : __BuiltinType, typename From : __BuiltinType, uint N>
vector<Target, N> ConvertBuiltInTo(vector<From, N> src)
{
	vector<Target, N> res;
	for(uint i = 0; i < N; ++i)
	{
		res[i] = ConvertBuiltInTo<Target>(src[i]);
	}
	return res;
}

template <typename T, uint N>
T sum(vector<T, N> v)
	where T : IArithmetic
{
	T res = {};
	for(uint i = 0; i < N; ++i)
	{
		res = res + v[i];
	}
	return res;
}

template <typename T>
T sqr(T x)
	where T : IArithmetic
{
	return x * x;
}


template <typename T, uint N>
T length2(vector<T, N> v)
	where T : __BuiltinFloatingPointType
{
	return dot(v, v);
}

template <typename T, uint N>
T length2(vector<T, N> v)
	where T : __BuiltinIntegerType
{
	return dot(v, v);
}

template <typename T, uint N>
float length(vector<T, N> v)
	where T : __BuiltinIntegerType
{
	const T l2 = length2(v);
	const float f = ConvertBuiltInTo<float>(l2);
	return sqrt(f);	
}

template <typename T : __BuiltinIntegerType>
T BitMask(T n)
{
	return (T(1) << n) - T(1);
}


template <typename Float : __BuiltinFloatingPointType, uint N>
vector<Float, N> safeNormalize(vector<Float, N> v)
{
	return length2(v) == Float(0) ? v : normalize(v);
}

f16 f16Offset(f16 f, int16_t o)
{
	i16 i = asint16(f);
	i += o;
	f = asfloat16(i);
	return f;
}

f32 f32Offset(f32 f, int32_t o)
{
	i32 i = asint(f);
	i += o;
	f = asfloat(i);
	return f;
}

f64 f64Offset(f64 f, int64_t o)
{
	i64 i = bit_cast<i64>(f);
	i += o;
	f = bit_cast<f64>(i);
	return f;
}

template <typename Float : __BuiltinFloatingPointType>
Float Luminance(vector<Float, 3> rgb)
{
	vector<Float, 3> p = {Float(0.299), Float(0.587), Float(0.114)};
	return dot(rgb, p);
}

template <typename Scalar : IArithmetic, uint N>
Scalar Average(vector<Scalar, N> v)
{
	return sum(v) / Scalar(N);
}

template <typename Scalar : IArithmetic>
Scalar Grey(vector<Scalar, 3> rgb)
{
	return Average(rgb);
}

template <typename Float : __BuiltinFloatingPointType>
bool IsWrong(Float f)
{
	return isnan(f) || isinf(f);
}

template <typename Float : __BuiltinFloatingPointType, uint N>
bvec<N> IsWrong(vector<Float, N> v)
{
	return isnan(v) || isinf(v);
}

template <typename Float : __BuiltinFloatingPointType>
Float FitWrongToZero(Float f)
{
	return IsWrong(f) ? Float(0) : f;
}

template <typename Float : __BuiltinFloatingPointType, uint N>
vector<Float, N> FitWrongToZero(vector<Float, N> v)
{
	for(uint i = 0; i < N; ++i)
	{
		v[i] = FitWrongToZero(v[i]);		
	}
	return v;
}

template <typename Float : __BuiltinFloatingPointType, uint Rows, uint Cols>
matrix<Float, Rows, Cols> FitWrongToZero(matrix<Float, Rows, Cols> m)
{
	for(uint i = 0; i < Rows; ++i)
	{
		m[i] = FitWrongToZero(m[i]);
	}
	return m;
}

template <typename T : __BuiltinFloatingPointType, uint N>
bool NonZero(Vector<T, N> v)
{
	T zero_t = {};
	Vector<T, N> z = Vector<T, N>(zero_t);
	return any(v != z);
}

template <typename T : __BuiltinIntegerType, uint N>
bool NonZero(Vector<T, N> v)
{
	T zero_t = {};
	Vector<T, N> z = Vector<T, N>(zero_t);
	return any(v != z);
}

#include "constants"

