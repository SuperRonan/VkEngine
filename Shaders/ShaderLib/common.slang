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

template <typename T>
typealias Matrix2 = Matrix<T, 2, 2>;
template <typename T>
typealias Matrix3 = Matrix<T, 3, 3>;
template <typename T>
typealias Matrix4 = Matrix<T, 4, 4>;

template <typename T>
typealias Matrix2x2 = Matrix2<T>;
template <typename T>
typealias Matrix3x3 = Matrix3<T>;
template <typename T>
typealias Matrix4x4 = Matrix4<T>;

template <typename T>
typealias Matrix2x3 = Matrix<T, 2, 3>;
template <typename T>
typealias Matrix2x4 = Matrix<T, 2, 4>;

template <typename T>
typealias Matrix3x2 = Matrix<T, 3, 2>;
template <typename T>
typealias Matrix3x4 = Matrix<T, 3, 4>;

template <typename T>
typealias Matrix4x2 = Matrix<T, 4, 2>;
template <typename T>
typealias Matrix4x3 = Matrix<T, 4, 3>;

template <uint R, uint C = R>
typealias Matrixf = Matrix<float, R, C>;

template <uint R, uint C = R>
typealias Matrixd = Matrix<double, R, C>;

template <uint R, uint C = R>
typealias Matrixh = Matrix<half, R, C>;

typealias Matrix2f = Matrix2<float>;
typealias Matrix3f = Matrix3<float>;
typealias Matrix4f = Matrix4<float>;

typealias Matrix2x3f = Matrix2x3<float>;
typealias Matrix2x4f = Matrix2x4<float>;

typealias Matrix3x2f = Matrix3x2<float>;
typealias Matrix3x4f = Matrix3x4<float>;

typealias Matrix4x2f = Matrix4x2<float>;
typealias Matrix4x3f = Matrix4x3<float>;

template <typename T, uint R, uint C>
Vector<T, R> GetColumn(const in Matrix<T, R, C> m, uint i)
{
	Vector<T, R> res;
	for(uint j = 0; j < R; ++j)
	{
		res[j] = m[j][i];
	}
	return res;
}

template <typename T, uint R, uint C>
Vector<T, C> GetRow(const in Matrix<T, R, C> m, uint i)
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

template <typename T, uint R, uint C>
T GetCoeficient(const in Matrix<T, R, C> m, uint r, uint c)
{
	return m[r][c];
}

template <typename T, uint R, uint C>
void SetCoeficient(inout Matrix<T, R, C> m, uint r, uint c, const in T t)
{
	m[r][c] = t;
}

template<typename T : __BuiltinFloatingPointType, uint R, uint I, uint C>
Matrix<T, R, C> operator*(const in Matrix<T, R, I> l, const in Matrix<T, I, C> r)
{
	return mul(l, r);
}

template <uint R, uint C = R, typename T : __BuiltinFloatingPointType = float>
Matrix<T, R, C> DiagonalMatrix(T d = T(1))
{
	Matrix<T, R, C> res = Matrix<T, R, C>(d);
	return res;
}

template <uint BlockR, uint BlockC, typename T : __BuiltinFloatingPointType, uint SrcR, uint SrcC>
Matrix<T, BlockR, BlockC> ExtractBlock(const in Matrix<T, SrcR, SrcC> m, uint2 offset)
	//where ((BlockR <= SrcR) && (BlockC <= SrcC))
{
	Matrix<T, BlockR, BlockC> res;
	for(uint j = 0; j < BlockC; ++j)
	{
		for(uint i = 0; i < BlockR; ++i)
		{
			SetCoeficient(res, i, j, GetCoeficient(m, offset.x + i, offset.y + j));
		}
	}
	return res;
}

template <uint BlockR, uint BlockC, typename T : __BuiltinFloatingPointType, uint DstR, uint DstC>
void SetBlock(inout Matrix<T, DstR, DstC> m, uint2 offset, const in Matrix<T, BlockR, BlockC> block)
	//where ((BlockR <= DstR) && (BlockC <= DstC))
{
	for(uint j = 0; j < BlockC; ++j)
	{
		for(uint i = 0; i < BlockR; ++i)
		{
			SetCoeficient(m, offset.x + i, offset.y + j, GetCoeficient(block, i, j));
		}
	}
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

template <typename T>
typealias Vector2 = Vector<T, 2>;

template <typename T>
typealias Vector3 = Vector<T, 3>;

template <typename T>
typealias Vector4 = Vector<T, 4>;

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
// m[i] returns the i-th row of the matrix (even if the matrices are configured to be col major?)

#if SLANG_USE_GLSL_MATRIX_TYPES
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
#endif


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

template <typename T : __BuiltinFloatingPointType, uint N>
T Dot(const in Vector<T, N> l, const in Vector<T, N> r)
{
	return dot(l, r);
}

template <typename T : __BuiltinIntegerType, uint N>
T Dot(const in Vector<T, N> l, const in Vector<T, N> r)
{
	return dot(l, r);
}

template <typename T : __BuiltinFloatingPointType, uint N>
T InnerProduct(const in Vector<T, N> l, const in Vector<T, N> r)
{
	return dot(l, r);
}

template <typename T : __BuiltinIntegerType, uint N>
T InnerProduct(const in Vector<T, N> l, const in Vector<T, N> r)
{
	return dot(l, r);
}

template <typename T : __BuiltinFloatingPointType, uint R, uint C>
Matrix<T, R, C> OuterProduct(const in Vector<T, R> l, const in Vector<T, C> r)
{
	Matrix<T, R, C> res;
	for(uint i = 0; i < R; ++i)
	{
		SetRow(res, i, l[i] * r);
	}
	return res;
}

template <typename T : __BuiltinIntegerType, uint R, uint C>
Matrix<T, R, C> OuterProduct(const in Vector<T, R> l, const in Vector<T, C> r)
{
	Matrix<T, R, C> res;
	for(uint i = 0; i < R; ++i)
	{
		SetRow(res, i, l[i] * r);
	}
	return res;
}

template <typename T : __BuiltinFloatingPointType>
Vector3<T> Cross(const in Vector3<T> l, const in Vector3<T> r)
{
	return cross(l, r);
}

template <typename T : __BuiltinIntegerType>
Vector3<T> Cross(const in Vector3<T> l, const in Vector3<T> r)
{
	return cross(l, r);
}

template <typename T : __BuiltinFloatingPointType, uint R, uint C>
Matrix<T, C, R> Transpose(const in Matrix<T, R, C> m)
{
	return transpose(m);
}

template <typename T : __BuiltinFloatingPointType, uint N>
T Determinant(const in Matrix<T, N, N> m)
{
	return determinant(m);
}

template <typename T : __BuiltinFloatingPointType, uint N>
T Trace(const in Matrix<T, N, N> m)
{
	T res = T(0);
	for(uint i = 0; i < N; ++i)
	{
		res += m[i][i];
	}
	return res;
}

template<typename T : __BuiltinFloatingPointType, uint N>
Matrix<T, N, N> Inverse(const in Matrix<T, N, N> m);

template<typename T : __BuiltinFloatingPointType>
Matrix2<T> Inverse(const in Matrix2<T> m)
{
	Matrix2<T> res;
	const T d = Determinant(m);
	if(d != T(0))
	{
		SetCoeficient(res, 0, 0, GetCoeficient(m, 1, 1) / d);
		SetCoeficient(res, 1, 1, GetCoeficient(m, 0, 0) / d);
		SetCoeficient(res, 0, 1, -GetCoeficient(m, 1, 0) / d);
		SetCoeficient(res, 1, 0, -GetCoeficient(m, 0, 1) / d);
	}
	return res;
}

template<typename T : __BuiltinFloatingPointType>
Matrix3<T> Inverse(const in Matrix3<T> m)
{
	Matrix3<T> res;
	const T d = Determinant(m);
	if(d != T(0))
	{
#if 1
		// Cayley Hamilton
		const T tr = Trace(m);
		const Matrix3<T> m2 = m * m;
		res = rcp(d) * (DiagonalMatrix<3, 3>(T(0.5) * (sqr(tr) - Trace(m2))) + tr * m + m2);
#else
		// cross produc / triple product
		for(uint i = 0; i < 3; ++i)
		{
			const uint x1 = (i + 1) % 3;
			const uint x2 = (i + 2) % 3;
			SetRow(res, i, Cross(GetColumn(m, x1), GetColumn(m, x2)));
		}
		res = res / d;
#endif
	}
	return res;
}

template<typename T : __BuiltinFloatingPointType>
Matrix4<T> Inverse(const in Matrix4<T> m)
{
	const Matrix2<T> A = ExtractBlock<2, 2>(m, uint2(0, 0));
	const Matrix2<T> B = ExtractBlock<2, 2>(m, uint2(2, 0));
	const Matrix2<T> C = ExtractBlock<2, 2>(m, uint2(0, 2));
	const Matrix2<T> D = ExtractBlock<2, 2>(m, uint2(2, 2));

	const Matrix2<T> P = Inverse(A - B * Inverse(D) * C);
	const Matrix2<T> Di = Inverse(D);

	Matrix4<T> res;
	SetBlock(res, uint2(0, 0), P);
	SetBlock(res, uint2(0, 2), -P * B * Di);
	SetBlock(res, uint2(2, 0), -Di * C * P);
	SetBlock(res, uint2(2, 2), Di + Di * C * P * B * Di);
	return res;
}



#include "constants"

