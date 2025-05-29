#pragma once

#include "common.slang"

template <T : ITexelElement, int SampleCount, int Access, int IsShadow, int IsCombined, int Format>
uvec2 TextureSize(const in _Texture<T, __Shape2D, 0, 0, SampleCount, Access, IsShadow, IsCombined, Format> texture)
{
	uvec2 res;
	texture.GetDimensions(res.x, res.y);
	return res;
}

float TextureLODOffset(vec2 grad_x, vec2 grad_y)
{
	float delta_max_sqr = Max(Length2(grad_x), Length2(grad_y));
	return 0.5 * log2(delta_max_sqr);
}

[require(fragment)]
float TextureLODOffsetInQuad(vec2 uv)
{
	vec2 gx = ddx(uv);
	vec2 gy = ddy(uv);
	return TextureLODOffset(gx, gy);
}

interface ITextureSamplingInfo
{
	template <typename T : ITexelElement>
	T sample(const in Sampler2D<T> txt, vec2 uv);
};

struct DefaultTextureSampling : ITextureSamplingInfo
{
	float offset = 0;

	__init(float offset = 0)
	{
		this.offset = offset;
	}
	
	template <typename T : ITexelElement>
	T sample(const in Sampler2D<T> txt, vec2 uv)
	{
		return txt.SampleLevel(uv, offset);
	}
};

[require(fragment)]
[require(compute, SPV_KHR_compute_shader_derivatives)]
struct QuadDiffTextureSampling : ITextureSamplingInfo
{
	__init()
	{}

	template <typename T : ITexelElement>
	T sample(const in Sampler2D<T> txt, vec2 uv)
	{
		return txt.Sample(uv);
	}
};

struct RayDiffTextureSampling : ITextureSamplingInfo
{
	Matrix2f uv_jacobian = Matrix2f::Zero();

	__init(const in Matrix2f uv_jacobian)
	{
		this.uv_jacobian = uv_jacobian;
	}

	template <typename T : ITexelElement>
	T sample(const in Sampler2D<T> txt, vec2 uv)
	{
		return txt.SampleGrad(uv, GetColumn(uv_jacobian, 0), GetColumn(uv_jacobian, 1));
	}
};

