#pragma once

#include <ShaderLib/interop_slang_cpp>

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
struct AsymetricGaussian SLANG_ONLY(: IFunc<Float, Float>)
{
	Float mu;
	Float t1, t2;

#if __cplusplus
	constexpr AsymetricGaussian(Float mu, Float t1, Float t2) :
		mu(mu),
		t1(t1),
		t2(t2)
	{}
#elif _slang
	constexpr __init(Float mu, Float t1, Float t2)
	{
		this.mu = mu;
		this.t1 = t1;
		this.t2 = t2;
	}
#endif

	constexpr Float operator()(Float x)
	{
		Float t = x < mu ? t1 : t2;
		Float res = CPP_ONLY(std::)exp(-sqr(t * (x - mu)) * Float(0.5));
		return res;
	}
};