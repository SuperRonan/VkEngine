#pragma once

#include <ShaderLib/common.slang>

struct AsymetricGaussian<Float : __BuiltinFloatingPointType> : IFunc<Float, Float>
{
	Float mu;
	Float t1, t2;

	__init(Float mu, Float t1, Float t2)
	{
		this.mu = mu;
		this.t1 = t1;
		this.t2 = t2;
	}

	Float operator()(Float x)
	{
		Float t = x < mu ? t1 : t2;
		Float res = exp(-sqr(t * (x - mu)) * Float(0.5));
		return res;
	}
}