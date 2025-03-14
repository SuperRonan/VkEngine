#pragma once

#include <ShaderLib/Maths/NumericLimits.slang>


__generic<typename T, uint64_t Address_Space, uint Compile_Time_Size = uint(-1)>
struct Span : IRWArray<T>
{
	Ptr<T, Address_Space> data;

	__init(const in Ptr<T, Address_Space> data)
	{
		this.data = data;
	}
	
	__subscript(uint i) -> T
	{
		get {
			return data[i];
		}
		set {
			data[i] = newValue;
		}
	}

	uint getCount()
	{
		return Compile_Time_Size;
	}
};

__generic<typename T, uint64_t Address_Space>
extension Span<T, Address_Space, uint(-1)>
{
	uint size = 0;

	__init(const in Ptr<T, Address_Space> data, uint size)
	{
		this.data = data;
		this.size = size;
	}

	[OverloadRank(15)]
	uint getCount()
	{
		return size;
	}
};

__generic<typename T, uint64_t Address_Space>
Span<T, Address_Space> MakeSpan(Ptr<T, Address_Space> ptr, uint size)
{
	Span<T, Address_Space> res = Span<T, Address_Space>(ptr, size);
	return res;
}

__generic<uint Compile_Time_Size, typename T, uint64_t Address_Space>
Span<T, Address_Space, Compile_Time_Size> MakeSpan(Ptr<T, Address_Space> ptr)
{
	static_assert(Compile_Time_Size != uint(-1), "The compile time size must not be uint(-1)");
	Span<T, Address_Space, Compile_Time_Size> res = {ptr};
	return res;
}

// I am not sure this is actually legal in SPIR-V
__generic<typename Q, typename T, uint64_t Address_Space>
Ptr<Q, Address_Space> ReinterpretPointer(Ptr<T, Address_Space> p)
{
	Ptr<Q, Address_Space> res = reinterpret<Ptr<Q, Address_Space>>(p);
	return res;
}


// __generic<typename T, uint64_t Address_Space>
// RWSpan<T, Address_Space> MakeRWSpan(Ptr<T, Address_Space> ptr, uint size)
// {
// 	RWSpan<T, Address_Space> res;
// 	res.data = ptr;
// 	res.size = size;
// 	return res;
// }
// __generic<uint Compile_Time_Size, typename T, uint64_t Address_Space>
// RWSpan<T, Address_Space, Compile_Time_Size> RWMakeSpan(Ptr<T, Address_Space> ptr)
// {
// 	static_assert(Compile_Time_Size != uint(-1), "The compile time size must not be uint(-1)");
// 	RWSpan<T, Address_Space, Compile_Time_Size> res;
// 	res.data = ptr;
// 	return res;
// }


