#pragma once

#include "common.slang"

template <T : ITexelElement>
uvec2 TextureSize(const in RWTexture2D<T> texture)
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
	
	template <typename T : ITexelElement>
	T sample(const in Sampler2D<T> txt, vec2 uv)
	{
		return txt.SampleLevel(uv, offset);
	}
};

[require(SPV_NV_compute_shader_derivatives)]
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
	vec2 grad_x = vec2(0);
	vec2 grad_y = vec2(0);

	// All expressed in TBN space
	[mutating]
	void init(const in Matrix3f TBN, const in vec3 view_dir, const in Matrix3x2f ray_diff)
	{
		const Matrix3f TBN_ = Transpose(TBN);
		const Matrix3x2f prod = TBN_ * ray_diff;
		grad_x = GetRow(prod, 0).xy;
		grad_y = GetRow(prod, 1).xy;
	}

	__init(const in Matrix3f TBN, vec3 view_dir, vec3 dx, vec3 dy)
	{
		init(TBN, view_dir, MakeFromCols(dx, dy));
	}

	template <typename T : ITexelElement>
	T sample(const in Sampler2D<T> txt, vec2 uv)
	{
		return txt.SampleGrad(uv, grad_x, grad_y);
	}
};

