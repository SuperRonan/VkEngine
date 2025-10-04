#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/Maths/Transforms.hpp>

namespace vkl
{
	enum class ColorCorrectionMode : uint32_t
	{
		PassThrough = 0,
		None = PassThrough,
		Id = None,
		ITU,
		sRGB,
		scRGB,
		BT1886,
		HybridLogGamma,
		HLG = HybridLogGamma,
		PerceptualQuantization,
		PQ = PerceptualQuantization,
		DisplayP3,
		DCI_P3,
		LegacyNTSC,
		LegacyPAL,
		ST240,
		AdobeRGB,
		SonySLog,
		SonySLog2,
		ACEScc,
		ACEScct,
		Gamma,
	};

	struct ColorCorrectionParams
	{
		float exposure = 1.0f;
		float gamma = 1.0f;
	};

	struct ColorCorrectionInfo
	{
		ColorCorrectionMode mode = ColorCorrectionMode::None;
		ColorCorrectionParams params = {};
	};

	extern ColorCorrectionInfo DeduceColorCorrection(VkSurfaceFormatKHR format);

	struct ColorCorrectionCommon
	{
// TODO properly
#define constexpr_26

		static constexpr_26 float RectifiedGammaTF(float linear_value, float gamma, float lambda, float beta, float alpha)
		{
			if (linear_value <= beta)	return linear_value * lambda;
			else						return (1 + alpha) * pow(linear_value, gamma) - alpha;
		}

		static constexpr_26 float RectifiedGammaBetaConstant(float gamma, float lambda, const uint iterations)
		{
			// Solve with Newton's tf, should be constexpr
			// Equation: 0 = b * (1 - rcp(g)) + rcp(g) * pow(b, 1 - g) - rcp(l)
			// Derivative: (1 - rcp(g)) * (1 - pow(b, -g))
			float beta = 0.5 * pow(rcp(lambda), rcp(1 - gamma));
			const float rg = rcp(gamma);
			for (uint i = 0; i < iterations; ++i)
			{
				const float f = beta * (1 - rg) + rg * pow(beta, 1 - gamma) - rcp(lambda);
				const float df = (1 - rg) * (1 - pow(beta, -gamma));
				beta -= f / df;
			}
			return beta;
		}

		static constexpr_26 float RectifiedGammaBetaConstant(float gamma, float lambda)
		{
			return RectifiedGammaBetaConstant(gamma, lambda, 4);
		}

		static constexpr_26 float RectifiedGammaAlphaConstant(float gamma, float lambda, float beta)
		{
			return lambda * rcp(gamma) * std::pow(beta, 1 - gamma) - 1;
		}

		static constexpr_26 float RectifiedGammaTF(float linear_value, float gamma, float lambda, float beta)
		{
			const float alpha = RectifiedGammaAlphaConstant(gamma, lambda, beta);
			return RectifiedGammaTF(linear_value, gamma, lambda, beta, alpha);
		}

		static constexpr_26 float RectifiedGammaTF(float linear_value, float gamma, float lambda)
		{
			const float beta = RectifiedGammaBetaConstant(gamma, lambda);
			return RectifiedGammaTF(linear_value, gamma, lambda, beta);
		}

		static constexpr_26 float sRGB_OETF(float l)
		{
			const float gamma = rcp(2.4f);
			const float lambda = 12.92f;
			const float beta = RectifiedGammaBetaConstant(gamma, lambda);
			//const float beta = 0.003041282560128;
			const float alpha = RectifiedGammaAlphaConstant(gamma, lambda, beta);// 0.05501;
			//return l <= beta ? lambda * l : ((1 + alpha) * pow(l, gamma) - alpha);
			return RectifiedGammaTF(l, gamma, lambda);
		}
	};
}