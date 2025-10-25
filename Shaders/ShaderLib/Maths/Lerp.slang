#pragma once

#include <ShaderLib/common.slang>

// `lerp` for `IFloat`
__generic <Float : IFloat, Interpolant : __BuiltinFloatingPointType>
Float Lerp(const in Float zero, const in Float one, Interpolant t, bool fast=true)
{
	if (fast)
		return zero.add(one.sub(zero).scale(t));
	else
		return zero.scale(Interpolant(1) - t).add(one.scale(t));
}

// `Lerp` for IFloat arrays / vectors
__generic <Float : IFloat>
Float LerpV(const in Float zero, const in Float one, const in Float t, bool fast=true)
{
	if (fast)
		return zero.add(one.sub(zero).mul(t));
	else
		return zero.mul(Float(1) - t).add(one.mul(t));
}

// `Lerp` in IArray
__generic <typename Float : IFloat, ArrayF : IArray<Float>>
Float Lerp(const in ArrayF array, float u, bool fast=true)
{
	if(array.getCount() == 0)
	{
		return {};
	}
	else if(array.getCount() == 1)
	{
		return array[0];
	}
	let f = floor(u);
	let n = int(u);
	let index = clamp(n, 0, array.getCount() - 1);
	float i = u - float(index);
	return Lerp(array[index], array[index + 1], i, fast);
}

