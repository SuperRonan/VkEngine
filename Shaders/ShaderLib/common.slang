#pragma once

// https://github.com/shader-slang/slang/issues/5303
//#pragma warning(disable: 15205)

#ifndef __cplusplus
#define __cplusplus 0
#endif

#ifndef SLANG_USE_GLSL_MATRIX_TYPES
#define SLANG_USE_GLSL_MATRIX_TYPES 0
#endif

#ifndef SHADER_SEMANTIC_RAYGEN
#define SHADER_SEMANTIC_RAYGEN 0
#endif

#ifndef SHADER_SEMANTIC_INTERSECTION
#define SHADER_SEMANTIC_INTERSECTION 0
#endif

#ifndef SHADER_SEMANTIC_ANY_HIT
#define SHADER_SEMANTIC_ANY_HIT 0
#endif

#ifndef SHADER_SEMANTIC_CLOSEST_HIT
#define SHADER_SEMANTIC_CLOSEST_HIT 0
#endif

#ifndef SHADER_SEMANTIC_MISS
#define SHADER_SEMANTIC_MISS 0
#endif

#ifndef SHADER_SEMANTIC_CALLABLE
#define SHADER_SEMANTIC_CALLABLE 0
#endif


#include "core"
#include "bindings"

#include "interop_slang_cpp"

typealias i8  = int8_t;
typealias i16 = int16_t;
typealias i32 = int32_t;
typealias i64 = int64_t;

typealias s8 = i8;
typealias s16 = i16;
typealias s32 = i32;
typealias s64 = i64;

typealias u8  = uint8_t;
typealias u16 = uint16_t;
typealias u32 = uint32_t;
typealias u64 = uint64_t;

typealias f16 = float16_t;
typealias f32 = float32_t;
typealias f64 = float64_t;


#define IS_BUILTIN_FLOAT(T) ((T is f32) || (T is f64) || (T is f16))
#define IS_BUILTIN_INTEGER_ANY_SIZE(T, Prefix) ((T is Prefix##8) || (T is Prefix##16) || (T is Prefix##32) || (T is Prefix##64))
#define IS_BUILTIN_SIGNED_INT(T) IS_BUILTIN_INTEGER_ANY_SIZE(T, s)
#define IS_BUILTIN_UNSIGNED_INT(T) IS_BUILTIN_INTEGER_ANY_SIZE(T, u)
#define IS_BUILTIN_INTEGER(T) (IS_BUILTIN_SIGNED_INT(T) || IS_BUILTIN_UNSIGNED_INT(T))
#define IS_BUILTIN_SIZED_ANY_INTEGER(T, SIZE) ((T is i##SIZE) || (T is u##SIZE))

#if SHADER_FP16_AVAILABLE
// typedef half fp16IFP
typealias fp16IFP = half;
#else
typealias fp16IFP = float;
#endif

template <typename T, int N>
typealias Vector = vector<T, N>;

template <typename T, int R, int C, int L = 0>
typealias Matrix = matrix<T, R, C, L>;

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

template <int R, int C = R>
typealias Matrixf = Matrix<float, R, C>;

template <int R, int C = R>
typealias Matrixd = Matrix<double, R, C>;

template <int R, int C = R>
typealias Matrixh = Matrix<half, R, C>;

typealias Matrix2f = Matrix2<float>;
typealias Matrix3f = Matrix3<float>;
typealias Matrix4f = Matrix4<float>;

typealias Matrix2x2f = Matrix2<float>;
typealias Matrix3x3f = Matrix3<float>;
typealias Matrix4x4f = Matrix4<float>;

typealias Matrix2x3f = Matrix2x3<float>;
typealias Matrix2x4f = Matrix2x4<float>;

typealias Matrix3x2f = Matrix3x2<float>;
typealias Matrix3x4f = Matrix3x4<float>;

typealias Matrix4x2f = Matrix4x2<float>;
typealias Matrix4x3f = Matrix4x3<float>;

__generic <typename T : __BuiltinFloatingPointType, int R, int C>
extension matrix<T, R, C>
{
	static matrix<T, R, C> Uniform(const in T t)
	{
		matrix<T, R, C> res;
		for(uint i = 0; i < R; ++i)
		{
			for(uint j = 0; j < C; ++j)
			{
				res[i][j] = t;
			}
		}
		return res;
	}

	static matrix<T, R, C> Zero()
	{
		T z = T(0);
		return Uniform(z);
	}

	property t : matrix<T, C, R>
	{
		get{
			return transpose(this);
		}
	}
};

template <typename T, int R, int C>
Vector<T, R> GetColumn(const in Matrix<T, R, C> m, uint i)
{
	Vector<T, R> res;
	for(uint j = 0; j < R; ++j)
	{
		res[j] = m[j][i];
	}
	return res;
}

template <typename T, int R, int C>
Vector<T, C> GetRow(const in Matrix<T, R, C> m, uint i)
{
	return m[i];
}

template <typename T, int R, int C>
void SetColumn(inout Matrix<T, R, C> m, uint i, Vector<T, R> v)
{
	for(uint j = 0; j < R; ++j)
	{
		m[j][i] = v[j];
	}
}

template <typename T, int R, int C>
void SetRow(inout Matrix<T, R, C> m, uint i, Vector<T, C> v)
{
	m[i] = v;
}

template <typename T, int R, int C>
T GetCoeficient(const in Matrix<T, R, C> m, uint r, uint c)
{
	return m[r][c];
}

template <typename T, int R, int C>
void SetCoeficient(inout Matrix<T, R, C> m, uint r, uint c, const in T t)
{
	m[r][c] = t;
}

__generic<typename T : __BuiltinFloatingPointType, int R, int C>
public Matrix<T, R, C> MultCWise(const in Matrix<T, R, C> a, const in Matrix<T, R, C> b)
{
	// before the * operator is overloaded
	return a * b; 
}

[OverloadRank(15)]
[ForceInline]
public Matrix<float, N, N> operator*<let N:int>(const in matrix<float, N, N> m1, const in matrix<float, N, N> m2)
{
	return mul(m1, m2);
}

[OverloadRank(15)]
[ForceInline]
public Matrix<half, N, N> operator*<let N:int>(const in matrix<half, N, N> m1, const in matrix<half, N, N> m2)
{
	return mul(m1, m2);
}

[OverloadRank(15)]
[ForceInline]
public Matrix<double, N, N> operator*<let N:int>(const in matrix<double, N, N> m1, const in matrix<double, N, N> m2)
{
	return mul(m1, m2);
}

[ForceInline]
[OverloadRank(15)]
public Matrix<T, R, C> operator*<T:__BuiltinFloatingPointType, let L : int, let C : int, let R : int>(const in matrix<T, R, L> m1, const in matrix<T, L, C> m2)
{
	return mul(m1, m2);
}


// [ForceInline]
// [OverloadRank(15)]
// template <typename T :__BuiltinFloatingPointType, int R, int C>
// public Vector<T, C> operator*(const in T v, const in Matrix<T, R, C> m)
// {
// 	return mul(v, m);
// }

// // This one breaks scalar * matrix, but why?
// [ForceInline]
// //[OverloadRank(15)]
// template <typename T :__BuiltinFloatingPointType, int R, int C>
// public Vector<T, C> operator*(const in Vector<T, R> v, const in Matrix<T, R, C> m)
// {
// 	return mul(v, m);
// }

[ForceInline]
//[OverloadRank(15)]
public Vector<T, R> operator*<T:__BuiltinFloatingPointType, let C : int, let R : int>(const in matrix<T, R, C> m, const in vector<T, C> v)
{
	return mul(m, v);
}

// [ForceInline]
// [OverloadRank(15)]
// public Matrix<T, R, C> operator*<T:__BuiltinFloatingPointType, let R : int, let C : int>(const in Matrix<T, R, C> m, T t)
// {
// 	return mul(m, t);
// }

// By default, Slang defines the product operator * between two matrices of the same size with a cwise product
// But we prefer reserving the product operator * for inner product (classic matrix multiplication), since it is used much more often
// Ideally, the language should not impose its choice and let the user decide.
// Defining the * operator as we want it can easily be done above, but for the multiplication of two square matrices, the cwise mul is linked


// [OverloadRank(15)]
// [ForceInline]
// //[require(cpp_cuda_glsl_hlsl_spirv, sm_4_0_version)]
// __generic<int N>
// public matrix<float, N, N> matmul(matrix<float, N, N> m1, matrix<float, N, N> m2)
// {
// 	return mul(m1, m2);
// }




template <int R, int C = R, typename T : __BuiltinArithmeticType>
Matrix<T, R, C> DiagonalMatrix(T d = T(1))
{
	Matrix<T, R, C> res = Matrix<T, R, C>(T(0));
	for(uint i = 0; i < min(R, C); ++i)
	{
		res[i][i] = d;
	}
	return res;
}

template <int N, typename T : __BuiltinArithmeticType>
Matrix<T, N, N> DiagonalMatrixV(const in Vector<T, N> diag)
{
	Matrix<T, N, N> res = Matrix<T, N, N>(0);
	for(uint i = 0; i < N; ++i)
	{
		res[i][i] = diag[i];
	}
	return res;
}

template <int N, typename Scalar>
Vector<Scalar, N> MakeUniformVector(Scalar v)
{
	Vector<Scalar, N> res = Vector<Scalar, N>(v);
	return res;
}

template <int R, int C = R, typename Scalar>
Matrix<Scalar, R, C> MakeUniformMatrix(Scalar v)
{
	Matrix<Scalar, R, C> res;
	for(uint i = 0; i < R; ++i)
	{
		SetRow(res, i, MakeUniformVector<C>(v));
	}
	return res;
}

template <int BlockR, int BlockC, typename T : __BuiltinArithmeticType, int SrcR, int SrcC>
Matrix<T, BlockR, BlockC> ExtractBlock(const in Matrix<T, SrcR, SrcC> m, uint2 offset)
	//where ((BlockR <= SrcR) && (BlockC <= SrcC))
{
	Matrix<T, BlockR, BlockC> res = MakeUniformMatrix<BlockR, BlockC>(T(0));
	const uint2 ioffset = uint2(offset);
	const uint2 iBlock = uint2(BlockR, BlockC);
	const uint2 iSrc = uint2(SrcR, SrcC);
	const uint2 imax = min(ioffset + iBlock, iSrc);
	const uint2 irange = imax - ioffset; 
	for(uint i = 0; i < irange.x; ++i)
	{
		for(uint j = 0; j < irange.y; ++j)
		{
			SetCoeficient(res, i, j, GetCoeficient(m, offset.x + i, offset.y + j));
		}
	}
	return res;
}

template <int BlockR, int BlockC, typename T : __BuiltinArithmeticType, int SrcR, int SrcC>
Matrix<T, BlockR, BlockC> ExtractBlock(const in Matrix<T, SrcR, SrcC> m, uint row_offset = 0, uint col_offset = 0)
{
	return ExtractBlock<BlockR, BlockC>(m, uint2(row_offset, col_offset));
}

template <int BlockR, int BlockC, typename T, int DstR, int DstC>
void SetBlock(inout Matrix<T, DstR, DstC> m, uint2 offset, const in Matrix<T, BlockR, BlockC> block)
	//where ((BlockR <= DstR) && (BlockC <= DstC))
{
	const uint2 ioffset = uint2(offset);
	const uint2 iBlock = uint2(BlockR, BlockC);
	const uint2 iDst = uint2(DstR, DstC);
	const uint2 imax = min(iBlock, iDst + ioffset);
	const uint2 irange = imax - ioffset; 
	for(uint i = 0; i < irange.x; ++i)
	{
		for(uint j = 0; j < irange.y; ++j)
		{
			SetCoeficient(m, offset.x + i, offset.y + j, GetCoeficient(block, i, j));
		}
	}
}

template <int BlockR, int BlockC, typename T, int DstR, int DstC>
void SetBlock(inout Matrix<T, DstR, DstC> m, uint row_offset, uint col_offset, const in Matrix<T, BlockR, BlockC> block)
{
	SetBlock(m, uint2(row_offset, col_offset), block);
}

template <int BlockR, int BlockC, typename T, int DstR, int DstC>
void SetBlock(inout Matrix<T, DstR, DstC> m, const in Matrix<T, BlockR, BlockC> block)
{
	SetBlock(m, uint2(0, 0), block);
}

template <typename T, int R, int C>
Matrix<T, R, C> MakeFromRows(const in Vector<T, C>[R] rows)
{
	Matrix<T, R, C> res;
	for(uint i = 0; i < R; ++i)
	{
		SetRow(res, i, rows[i]);
	}
	return res;
}

template <typename T, int N>
Matrix<T, 2, N> MakeFromRows(const in Vector<T, N> r0, const in Vector<T, N> r1)
{
	Vector<T, N> rows[] = {r0, r1};
	return MakeFromRows(rows);
}

template <typename T, int N>
Matrix<T, 3, N> MakeFromRows(const in Vector<T, N> r0, const in Vector<T, N> r1, const in Vector<T, N> r2)
{
	Vector<T, N> rows[] = {r0, r1, r2};
	return MakeFromRows(rows);
}

template <typename T, int N>
Matrix<T, 4, N> MakeFromRows(const in Vector<T, N> r0, const in Vector<T, N> r1, const in Vector<T, N> r2, const in Vector<T, N> r3)
{
	Vector<T, N> rows[] = {r0, r1, r2, r3};
	return MakeFromRows(rows);
}



template <typename T, int R, int C>
Matrix<T, R, C> MakeFromCols(const in Vector<T, R>[C] cols)
{
	Matrix<T, R, C> res;
	for(uint i = 0; i < C; ++i)
	{
		SetColumn(res, i, cols[i]);
	}
	return res;
}

template <typename T, int N>
Matrix<T, N, 2> MakeFromCols(const in Vector<T, N> c0, const in Vector<T, N> c1)
{
	Vector<T, N> cols[] = {c0, c1};
	return MakeFromCols(cols);
}

template <typename T, int N>
Matrix<T, N, 3> MakeFromCols(const in Vector<T, N> c0, const in Vector<T, N> c1, const in Vector<T, N> c2)
{
	Vector<T, N> cols[] = {c0, c1, c2};
	return MakeFromCols(cols);
}

template <typename T, int N>
Matrix<T, N, 4> MakeFromCols(const in Vector<T, N> c0, const in Vector<T, N> c1, const in Vector<T, N> c2, const in Vector<T, N> c3)
{
	Vector<T, N> cols[] = {c0, c1, c2, c3};
	return MakeFromCols(cols);
}

template <int R, int C = R, typename T : __BuiltinArithmeticType, int SrcR, int SrcC>
Matrix<T, R, C> ResizeMatrix(const in Matrix<T, SrcR, SrcC> m, T diag = T(1))
{
	Matrix<T, R, C> res = ExtractBlock<R, C>(m, uint2(0, 0));
	const uint Src_square_N = min(SrcR, SrcC);
	const uint Dst_square_N = min(R, C);
	const uint N = Dst_square_N - Src_square_N;
	for(uint i = 0; i < N; ++i)
	{
		SetCoeficient(res, Src_square_N + i, Src_square_N + i, diag);
	}
	return res;
}


template <int N>
typealias vec = vector<float, N>;
template <int N>
typealias Vectorf = vector<float, N>;
typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;

template <int N>
typealias ivec = vector<int, N>;
typealias ivec2 = int2;
typealias ivec3 = int3;
typealias ivec4 = int4;

template <int N>
typealias uvec = vector<uint, N>;
typealias uvec2 = uint2;
typealias uvec3 = uint3;
typealias uvec4 = uint4;

template <int N>
typealias bvec = vector<bool, N>;
typealias bvec2 = bool2;
typealias bvec3 = bool3;
typealias bvec4 = bool4;

template <int N>
typealias dvec = vector<bool, N>;
typealias dvec2 = double2;
typealias dvec3 = double3;
typealias dvec4 = double4;

template <int N>
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
template <int Colums, int Rows>
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


typealias fp16vecIFP<int N> = vector<fp16IFP, N>;
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
constexpr Dst ConvertBuiltInTo_ ## Dst(From i) \
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
constexpr Target ConvertBuiltInTo(From i)
{
	Target res;
	if(false){}
	FOR_EACH_BUILTIN_TYPE(CONVERT_BUILTIN_CASE_2)
	return res;
}

template <typename Target : __BuiltinType, typename From : __BuiltinType, int N>
constexpr vector<Target, N> ConvertBuiltInTo(vector<From, N> src)
{
	vector<Target, N> res;
	for(uint i = 0; i < N; ++i)
	{
		res[i] = ConvertBuiltInTo<Target>(src[i]);
	}
	return res;
}

template <typename T>
T sqr(T x)
	where T : __BuiltinArithmeticType
{
	return x * x;
}

__generic<T : __BuiltinArithmeticType>
T SignedSqr(T x)
{
	T res = sqr(x);
	if(x < T(0))	res = -res;
	return res;
}


template <typename T : __BuiltinFloatingPointType, int N>
T Length2(vector<T, N> v)
{
	return dot(v, v);
}

template <typename T : __BuiltinIntegerType, int N>
T Length2(vector<T, N> v)
{
	return dot(v, v);
}

template <typename T : __BuiltinIntegerType, int N>
float Length(vector<T, N> v)
{
	const T l2 = Length2(v);
	const float f = ConvertBuiltInTo<float>(l2);
	return sqrt(f);	
}

template <typename T : __BuiltinFloatingPointType, int N>
T Length(const in Vector<T, N> v)
{
	return length(v);
}

template <typename T : __BuiltinIntegerType>
T BitMask(T n)
{
	return (T(1) << n) - T(1);
}

template <typename T : __BuiltinFloatingPointType, int N>
Vector<T, N> Normalize(const in Vector<T, N> v)
{
	return normalize(v);
}

template <typename T : __BuiltinFloatingPointType, int N>
Vector<T, N> Normalize2(const in Vector<T, N> v)
{
	return v / Length2(v);
}

template <typename Float : __BuiltinFloatingPointType, int N>
vector<Float, N> SafeNormalize(vector<Float, N> v)
{
	return Length2(v) == Float(0) ? v : Normalize(v);
}


// min/max/abs are only implemented for built-in int and float
// TODO the cwise version is not linked (with vec3 for example)

template <typename T : IComparable>
T Min(const in T a, const in T b)
{
	return (a < b) ? a : b;
}

template <typename T : IComparable>
T Max(const in T a, const in T b)
{
	return (a > b) ? a : b;
}

[OverloadRank(15)]
template <typename T : IComparable, typename Array : IRWArray<T>>
Array Min(const in Array a, const in Array b)
{
	Array res;
	for(uint i = 0; i < a.getCount(); ++i)
	{
		res[i] = Min(a[i], b[i]);
	}
	return res;
}

[OverloadRank(15)]
template <typename T : IComparable, typename Array : IRWArray<T>>
Array Max(const in Array a, const in Array b)
{
	Array res;
	for(uint i = 0; i < a.getCount(); ++i)
	{
		res[i] = Max(a[i], b[i]);
	}
	return res;
}

template <typename T : IArithmetic>
T Abs(const in T t)
{
	return (t >= T(0)) ? t : -t;
}

template <typename T : IArithmetic, typename Array : IRWArray<T>>
Array Abs(const in Array arr)
{
	Array res;
	for(uint i = 0; i < arr.getCount(); ++i)
	{
		res[i] = Abs(arr[i]);
	}
	return res;
}

__generic <typename T>
interface IReductor : IFunc<T, T, T>
{};

__generic <typename T>
interface IAssociativeReductor : IReductor<T>
{};

__generic <typename T : IArithmetic>
struct SumReductor : IAssociativeReductor<T>
{
	__init(){}
	T operator()(const in T a, const in T b)
	{
		return a + b;
	}
};

__generic <typename T : IArithmetic>
struct ProductReductor : IAssociativeReductor<T>
{
	__init(){}
	T operator()(const in T a, const in T b)
	{
		return a * b;
	}
};

__generic <typename T : IComparable>
struct MaxReductor : IAssociativeReductor<T>
{
	__init(){}
	T operator()(const in T a, const in T b)
	{
		return Max(a, b);
	}
};

__generic <typename T : IComparable>
struct MinReductor : IAssociativeReductor<T>
{
	__init(){}
	T operator()(const in T a, const in T b)
	{
		return Min(a, b);
	}
};

template <typename T>
void LinearRedux(const in IArray<T> array, inout T res, const in IReductor<T> reductor, uint offset, uint count)
{
	for(uint i = 0; i < count; ++i)
	{
		res = reductor(res, array[i + offset]);
	}
}

template <typename T>
void LinearRedux(const in IArray<T> array, inout T res, const in IReductor<T> reductor)
{
	LinearRedux(array, res, reductor, 0, array.getCount());
}

// TODO dichotomic redux 

template <typename T : IArithmetic>
T Sum(const in IArray<T> arr)
{
	T res = T(0);
	LinearRedux(arr, res, SumReductor<T>());
	return res;
}

template <typename T : IArithmetic>
T Prod(const in IArray<T> arr)
{
	T res = T(1);
	LinearRedux(arr, res, ProductReductor<T>());
	return res;
}

template <typename T : IComparable>
T Max(const in IArray<T> arr)
{
	T res = arr[0];
	LinearRedux(arr, res, MaxReductor<T>(), 1, arr.getCount() - 1);
	return res;
}

template <typename T : IComparable>
T Min(const in IArray<T> arr)
{
	T res = arr[0];
	LinearRedux(arr, res, MinReductor<T>(), 1, arr.getCount() - 1);
	return res;
}

template <typename Float : __BuiltinFloatingPointType>
Float Luminance(vector<Float, 3> rgb)
{
	vector<Float, 3> p = {Float(0.299), Float(0.587), Float(0.114)};
	return dot(rgb, p);
}

template <typename Scalar : __BuiltinArithmeticType, int N>
Scalar Average(vector<Scalar, N> v)
{
	return Sum(v) / Scalar(N);
}

template <typename Scalar : __BuiltinArithmeticType>
Scalar Grey(vector<Scalar, 3> rgb)
{
	return Average(rgb);
}

template <typename Float : __BuiltinFloatingPointType>
bool IsWrong(Float f)
{
	return isnan(f) || isinf(f);
}

template <typename Float : __BuiltinFloatingPointType, int N>
bvec<N> IsWrong(vector<Float, N> v)
{
	return isnan(v) || isinf(v);
}

template <typename Float : __BuiltinFloatingPointType>
Float FitWrongToZero(Float f)
{
	return IsWrong(f) ? Float(0) : f;
}

template <typename Float : __BuiltinFloatingPointType, int N>
vector<Float, N> FitWrongToZero(vector<Float, N> v)
{
	for(uint i = 0; i < N; ++i)
	{
		v[i] = FitWrongToZero(v[i]);		
	}
	return v;
}

template <typename Float : __BuiltinFloatingPointType, int Rows, int Cols>
matrix<Float, Rows, Cols> FitWrongToZero(matrix<Float, Rows, Cols> m)
{
	for(uint i = 0; i < Rows; ++i)
	{
		m[i] = FitWrongToZero(m[i]);
	}
	return m;
}

template <typename T : __BuiltinFloatingPointType, int N>
bool NonZero(Vector<T, N> v)
{
	T zero_t = {};
	Vector<T, N> z = Vector<T, N>(zero_t);
	return any(v != z);
}

template <typename T : __BuiltinIntegerType, int N>
bool NonZero(Vector<T, N> v)
{
	T zero_t = {};
	Vector<T, N> z = Vector<T, N>(zero_t);
	return any(v != z);
}

#define DOT_DISPATCH_IMPL(TYPE)	else if(T is TYPE)	{res = bit_cast<T>(dot(bit_cast<Vector<TYPE, N>>(bra), bit_cast<Vector<TYPE, N>>(ket)));}

template <typename T : __BuiltinArithmeticType, int N>
T Dot(const in Vector<T, N> bra, const in Vector<T, N> ket)
{
	T res;
	if(false) {}
	FOR_EACH_BUILTIN_TYPE(DOT_DISPATCH_IMPL)
	return res;
}

template <typename T : __BuiltinArithmeticType, int N>
T AbsDot(const in Vector<T, N> bra, const in Vector<T, N> ket)
{
	return Abs(Dot(bra, ket));
}

template <typename T : __BuiltinArithmeticType, int N>
T InnerProduct(const in Vector<T, N> l, const in Vector<T, N> r)
{
	return Dot(l, r);
}

template <typename T : __BuiltinArithmeticType, int R, int C>
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

template <typename T : __BuiltinFloatingPointType, int R, int C>
Matrix<T, C, R> Transpose(const in Matrix<T, R, C> m)
{
	return transpose(m);
}

template <typename T : __BuiltinFloatingPointType, int N>
T Determinant(const in Matrix<T, N, N> m)
{
	return determinant(m);
}

template <typename T : __BuiltinFloatingPointType, int N>
T Trace(const in Matrix<T, N, N> m)
{
	T res = T(0);
	for(uint i = 0; i < N; ++i)
	{
		res += m[i][i];
	}
	return res;
}

template<typename T : __BuiltinFloatingPointType>
Matrix2<T> Inverse2(const in Matrix2<T> m)
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
Matrix3<T> Inverse3(const in Matrix3<T> m)
{
	Matrix3<T> res;
	const T d = Determinant(m);
	if(d != T(0))
	{
#if 1
		// Cayley Hamilton
		const T tr = Trace(m);
		const Matrix3<T> m2 = mul(m, m);
		const Matrix3<T> trm = tr * m;
		res = rcp(d) * (DiagonalMatrix<3, 3>(T(0.5) * (sqr(tr) - Trace(m2))) - trm + m2);
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
Matrix4<T> Inverse4(const in Matrix4<T> m)
{
	// m = 
	// [A B]
	// [C D]
	const Matrix2<T> A = ExtractBlock<2, 2>(m, uint2(0, 0));
	const Matrix2<T> B = ExtractBlock<2, 2>(m, uint2(0, 2));
	const Matrix2<T> C = ExtractBlock<2, 2>(m, uint2(2, 0));
	const Matrix2<T> D = ExtractBlock<2, 2>(m, uint2(2, 2));

	const Matrix2<T> Di = Inverse2(D);
	const Matrix2<T> P = Inverse2(A - B * Di * C);

	Matrix4<T> res;
	SetBlock(res, uint2(0, 0), P);
	SetBlock(res, uint2(0, 2), -P * B * Di);
	SetBlock(res, uint2(2, 0), -Di * C * P);
	SetBlock(res, uint2(2, 2), Di + Di * C * P * B * Di);
	return res;
}

template <typename T : __BuiltinFloatingPointType, int N>
Matrix<T, N, N> Inverse(const in Matrix<T, N, N> m)
{
	Matrix<T, N, N> res;
	if(N == 2)
	{
		res = bit_cast<Matrix<T, N, N>>(Inverse2(bit_cast<Matrix2<T>>(m)));
	}
	else if(N == 3)
	{
		res = bit_cast<Matrix<T, N, N>>(Inverse3(bit_cast<Matrix3<T>>(m)));
	}
	else if(N == 4)
	{
		res = bit_cast<Matrix<T, N, N>>(Inverse4(bit_cast<Matrix4<T>>(m)));
	}
	if(N > 4)
	{
		const int pivot = N / 2;
		const int remaining = N - pivot;

		// m = 
		// [A B]
		// [C D]
		
		const Matrix<T, pivot, pivot> A = ExtractBlock<pivot, pivot>(m, uint2(0, 0));
		const Matrix<T, pivot, remaining> B = ExtractBlock<pivot, remaining>(m, uint2(0, pivot));
		const Matrix<T, remaining, pivot> C = ExtractBlock<remaining, pivot>(m, uint2(pivot, 0));
		const Matrix<T, remaining, remaining> D = ExtractBlock<remaining, remaining>(m, uint2(pivot, pivot));

		const Matrix<T, remaining, remaining> Di = Inverse<T, remaining>(D);
		const Matrix<T, pivot, pivot> P = Inverse<T, pivot>(A - B * Di * C);

		SetBlock(res, uint2(0, 0), P);
		SetBlock(res, uint2(0, pivot), -P * B * Di);
		SetBlock(res, uint2(pivot, 0), -Di * C * P);
		SetBlock(res, uint2(pivot, pivot), Di + Di * C * P * B * Di);
	}
	else
	{
		
	}

	return res;
}

#include "constants"


__generic <Enum : __EnumType, int EnumBits, UInt : __BuiltinIntegerType = u32>
struct PackedEnumIndex
{
	UInt payload;

	__init()
	{
		payload = UInt(0);
	}

	__init(Enum e, UInt i = UInt(0))
	{
		static_assert((sizeof(UInt) * 8) > (EnumBits), "PackedEnumIndex does not have enough capacity!");
		payload = (i << EnumBits);
		payload = payload | (reinterpret<UInt>(e) & BitMask<UInt>(UInt(EnumBits)));
	}

	property Enum enum
	{
		get {
			return reinterpret<Enum>(payload & BitMask<UInt>(UInt(EnumBits)));
		}
		set {
			payload = bitfieldInsert(payload, reinterpret<UInt>(newValue), 0, EnumBits);
		}
	}

	property UInt index
	{
		get {
			return (payload >> EnumBits);
		}
		set {
			payload = bitfieldInsert(payload, newValue, EnumBits, 8 * sizeof(UInt) - EnumBits);
		}
	}
};


template <typename Scalar : __BuiltinFloatingPointType>
Scalar Radians(Scalar deg)
{
	return deg * (Scalar(PI) / Scalar(180));
}

// Currently, there is no way to select between a RW*Buffer and a *Buffer (read only) with the generics semantic
// We could proprose to have (like Textures with _Texture)
// _ByteAddressBuffer<Access>: 
// ByteAddressBuffer = _ByteAddressBuffer<0>
// RWByteAddressBuffer = _ByteAddressBuffer<1>
// _StructuredBuffer<T, L, Access>:
// StructuredBuffer<T, L> = _StructuredBuffer<T, L, 0>
// RWStructuredBuffer<T, L> = _StructuredBuffer<T, L, 1>
// It would be nice to also be able to express the writeonly access

#define ACCESS_READ_ONLY 0
#define ACCESS_READ_WRITE 1
#define ACCESS_WRITE_ONLY ACCESS_READ_WRITE

#define ACCESS_RO ACCESS_READ_ONLY
#define ACCESS_RW ACCESS_READ_WRITE
#define ACCESS_WO ACCESS_WRITE_ONLY

// Read-Only Memory Access
#define ROM ACCESS_RO
// Read-Write Memory Access
#define RWM ACCESS_RW
// Write-Only Memory Access
#define WOM ACCESS_WO

#define SLANG_ACCESS_PREFIX_0
#define SLANG_ACCESS_PREFIX_1 RW

#define SLANG_ACCESS_PREFIX(Access) CAT(SLANG_ACCESS_PREFIX_, Access)

#define BINDING_HANDLE(BASE_TYPE, Access) CAT(SLANG_ACCESS_PREFIX(Access), BASE_TYPE)



