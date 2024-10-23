#pragma once

#include <ShaderLib:/common.glsl>

// Khronos spec:	https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html
// Nanocolor lib: 	https://github.com/meshula/Nanocolor/tree/main
// VkColorSpaceKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkColorSpaceKHR.html

// Transfer functions
// OETF(x): Conversion from normalized linear light intensity of the scene (e.g. what a rendering engine should output, normalized because it might be pre-multiplied by an exposure factor) to a non linear electronic representation (to be encoded, like sRGB)
// EOTF(x): Conversion from a non linear electronic representation (decoded from sRGB) to a linear light intensity of the display (what light the screen produces)
// OOTF(x) = EOTF(OETF(x)) Conversion from the linear light intensity of the scene to the display linear light
// In some cases OETF and EOTF are perfect inverse of each other, and thus OOTF is the identity function.
// These transfer functions work with real numbers and do not include encoding or decoding (So even if EOTF and OETF are perfect inverse, OOTF is supposed to be a true Id, but because of the intermediate encoding / decoding step, there might be quantization / clamps)

// In practice, when color correcting the frame generated by the renderer to be displayed by the swapchain in a certain color space, we need to apply the OETF.
// There is just the case of sRGB formats, where the transfer function is applied by the hardware
// Most transfer functions apply the same transformation to each R, G and B channel. 

// No transfer function defined
#define KHR_DF_TRANSFER_UNSPECIFIED 0
// Linear transfer function (value proportional to intensity)
#define KHR_DF_TRANSFER_LINEAR      1
// Perceptually-linear transfer function of sRGH (~2.4)
#define KHR_DF_TRANSFER_SRGB        2
// Perceptually-linear transfer function of ITU non-HDR specifications (~1/.45)
#define KHR_DF_TRANSFER_ITU         3
// SMTPE170M (digital NTSC) defines an alias for the ITU transfer function (~1/.45)
#define KHR_DF_TRANSFER_SMTPE170M   3
// Perceptually-linear gamma function of original NTSC (simple 2.2 gamma)
#define KHR_DF_TRANSFER_NTSC        4
// Sony S-log used by Sony video cameras
#define KHR_DF_TRANSFER_SLOG        5
// Sony S-log 2 used by Sony video cameras
#define KHR_DF_TRANSFER_SLOG2       6
// ITU BT.1886 EOTF
#define KHR_DF_TRANSFER_BT1886      7
// ITU BT.2100 HLG OETF
#define KHR_DF_TRANSFER_HLG_OETF    8
// ITU BT.2100 HLG EOTF
#define KHR_DF_TRANSFER_HLG_EOTF    9
// ITU BT.2100 PQ EOTF
#define KHR_DF_TRANSFER_PQ_EOTF     10
// ITU BT.2100 PQ OETF
#define KHR_DF_TRANSFER_PQ_OETF     11
// DCI P3 transfer function
#define KHR_DF_TRANSFER_DCIP3       12
// Legacy PAL OETF
#define KHR_DF_TRANSFER_PAL_OETF    13
// Legacy PAL 625-line EOTF
#define KHR_DF_TRANSFER_PAL625_EOTF 14
// Legacy ST240 transfer function
#define KHR_DF_TRANSFER_ST240       15
// ACEScc transfer function
#define KHR_DF_TRANSFER_ACESCC      16
// ACEScct transfer function
#define KHR_DF_TRANSFER_ACESCCT     17
// Adobe RGB (1998) transfer function
#define KHR_DF_TRANSFER_ADOBERGB    18


#define TF_None 			0
#define TF_Id 				TF_None
#define TF_ITU 				(TF_None + 1)
#define TF_sRGB 			(TF_ITU + 1)
#define TF_scRGB 			(TF_sRGB + 1)
#define TF_BT1886 			(TF_scRGB + 1)
#define TF_HLG 				(TF_BT1886 + 1)
#define TF_PQ 				(TF_HLG + 1)
#define TF_DisplayP3		(TF_PQ + 1) // in RGB
#define TF_DCI_P3 			(TF_DisplayP3 + 1) // in XYZ
#define TF_Legacy_NTSC 		(TF_DCI_P3 + 1)
#define TF_Legacy_PAL 		(TF_Legacy_NTSC + 1)
#define TF_ST240 			(TF_Legacy_PAL + 1)
#define TF_AdobeRGB 		(TF_ST240 + 1)
#define TF_Sony_SLog 		(TF_AdobeRGB + 1)
#define TF_Sony_SLog2 		(TF_Sony_SLog + 1)
#define TF_ACEScc 			(TF_Sony_SLog2 + 1)
#define TF_ACEScct 			(TF_ACEScc + 1)
#define TF_Gamma			(TF_ACEScct + 1)

#define TF_MASK 0xFFFF

// https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#TRANSFER_ITU
float ITU_OETF(float l, float alpha, float beta)
{
	return (l < beta) ? (l * 4.5) : (alpha * pow(l, 0.45) - (alpha - 1));
}

float ITU_OETF(float l)
{
	// The alpha and beta factors are slightly off for 12 bit encoding
	const float alpha = 0.018053968510808;
	const float beta = 1.099296826809443;
	return ITU_OETF(l, alpha, beta);
}

float sRGB_OETF_pow(float l, float alpha, float inv_gamma)
{
	return (1 + alpha) * pow(l, inv_gamma) - alpha;
}

float sRGB_OETF(float l)
{
	const float alpha = 0.055;
	const float gamma = 2.4;
	const float t = 0.0031308;
	return (l <= t) ? (l * 12.92) : sRGB_OETF_pow(l, alpha, rcp(gamma));
}

float scRGB_OETF(float l)
{
	const float alpha = 0.055;
	const float gamma = 2.4;
	const float t = 0.0031308;
	float res;
	// Weird: can output negative values???
	if(l <= -t)
		res = -sRGB_OETF_pow(-l, alpha, rcp(gamma));
	else if(l < t)
		res = l * 12.92;
	else
		res = sRGB_OETF_pow(l, alpha, rcp(gamma));
	return res;
}

// l in [0, 1]
// for BT.2100-1 and BT.2100-2 (BT.2100-0 uses other constants)
float HLG_Normalized_OETF(float l)
{
	const float a = 0.17883277;
	const float b = 1 - 4 * a;
	const float c = 0.5 - a * log(4 * a);
	return (l <= rcp(12.0)) ? (sqrt(3 * l)) : (a * log(12 * l - b) + c);
}

float HLG_OETF(float l)
{
	return HLG_Normalized_OETF(l);
}

// l in [0, 12]
float HLG_UnNormalized_OETF(float l)
{
	return HLG_Normalized_OETF(l / 12.0f);
}

float PQ_OETF(float l);

float FitPQ_Gamma_Linear(float exposure)
{
	const float alpha = 0.005182;
	const float beta = 3.333;
	const float gamma = alpha * exposure + beta;
	return gamma;
}

float FitPQGamma_Pow(float exposure)
{	
	const float a = 2.543;
	const float b = 0.1204;
	const float c = 0;
	
	// const float a = 1.39364;
	// const float b = 0.175651;
	// const float c = 1.23851;
	
	const float gamma = a * pow(exposure, b) + c;
	return gamma;
}

// Assume l is pre mult by exposure
float PQ_FitWithGamma_OETF(float l, float exposure)
{
	// Approx linear fit of gamma depending on exposure
	const float e = PQ_OETF(exposure);
	const float gamma = FitPQGamma_Pow(exposure);
	return e * pow(l / exposure, rcp(gamma));
}

// l in [0, 1]
float PQ_OETF(float l)
{
	const float m1 = 1305.0 / 8192.0;
	const float m2 = 2523.0 / 32.0;
	const float c1 = 107.0 / 128.0;
	const float c2 = 2413.0 / 128.0;
	const float c3 = 2392.0 / 128.0;
	const float Y = l * rcp(10000.0);
	const float Ym1 = pow(Y, m1);
	return pow((c1 + c2 * Ym1) / (1 + c3 * Ym1), m2);
}

float DisplayP3_OETF(float l)
{
	return sRGB_OETF(l);
}

float DCI_P3_OETF(float l)
{
	const float gamma = 2.6;
	return pow(l / 52.37, rcp(gamma));
}

float Gamma_OETF(float l, float gamma)
{
	return pow(l, gamma);
}

float OETF(float l, uint method)
{
	float res = l;
	if(method == TF_ITU)
		res = ITU_OETF(l);
	else if(method == TF_sRGB)
		res = sRGB_OETF(l);
	else if(method == TF_scRGB)
		res = scRGB_OETF(l);
	// else if(method == TF_BT1886)
	// 	res = BT1886_OETF(l);
	else if(method == TF_HLG)
		res = HLG_OETF(l);
	else if(method == TF_PQ)
		res = PQ_OETF(l);
	else if (method == TF_DisplayP3)
		res = DisplayP3_OETF(l);
	else if(method == TF_DCI_P3)
		res = DCI_P3_OETF(l);
	return res;
}

float OETF(float l, uint method, float gamma)
{
	float res = l;
	if(method == TF_Gamma)
	{
		res = Gamma_OETF(l, gamma);
	}
	else
	{
		res = OETF(l, method);
	}
	return res;
}

vec3 OETF(vec3 linear_rgb, uint method, float gamma)
{
	vec3 res;
	for(uint i = 0; i < 3; ++i)
	{
		res[i] = OETF(linear_rgb[i], method, gamma);
	}
	return res;
}

// The Alpha channel is passed through
vec4 OETF(vec4 linear_rgb_alpha, uint method, float gamma)
{
	return vec4(OETF(linear_rgb_alpha.rgb, method, gamma), linear_rgb_alpha.a);
}



