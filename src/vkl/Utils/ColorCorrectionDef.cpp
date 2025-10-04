#include <vkl/Utils/ColorCorrectionDef.hpp>

#include <vkl/VkObjects/DetailedVkFormat.hpp>

namespace vkl
{
	ColorCorrectionInfo DeduceColorCorrection(VkSurfaceFormatKHR format)
	{
		const DetailedVkFormat detailed_format = DetailedVkFormat::Find(format.format);

		ColorCorrectionInfo res = {};
		ColorCorrectionMode& _mode = res.mode;
		float& _gamma = res.params.gamma;
		float& _exposure = res.params.exposure;

		_gamma = 1.0f;
		_exposure = 1.0f;

		switch (format.colorSpace)
		{
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: {
			if (detailed_format.color.type == DetailedVkFormat::Type::SRGB)
			{
				_mode = ColorCorrectionMode::None;
				_gamma = 1.0f;
			}
			else
			{
				_mode = ColorCorrectionMode::sRGB;
				_gamma = 1.0 / 2.4;
			}
		} break;
		case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT: {
			_mode = ColorCorrectionMode::DisplayP3;
		} break;
		case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: {
			_mode = ColorCorrectionMode::None;
			if (detailed_format.color.type == DetailedVkFormat::Type::SFLOAT || detailed_format.color.type == DetailedVkFormat::Type::UFLOAT)
			{
				_exposure *= 3;
			}
		} break;
		case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT: {
			_mode = ColorCorrectionMode::PassThrough; // TODO check
		} break;
		case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT: {
			_mode = ColorCorrectionMode::DCI_P3;
		} break;
		case VK_COLOR_SPACE_BT709_LINEAR_EXT: {
			_mode = ColorCorrectionMode::PassThrough; // TODO check
		} break;
		case VK_COLOR_SPACE_BT709_NONLINEAR_EXT: {
			_mode = ColorCorrectionMode::ITU;
			_gamma = 1.0 / 2.2;
		} break;
		case VK_COLOR_SPACE_BT2020_LINEAR_EXT: {
			_mode = ColorCorrectionMode::PassThrough; // TODO check
		} break;
		case VK_COLOR_SPACE_HDR10_ST2084_EXT: {
			_mode = ColorCorrectionMode::PerceptualQuantization;
			_exposure *= 128 * 2;
		} break;
		case VK_COLOR_SPACE_DOLBYVISION_EXT: {
			_mode = ColorCorrectionMode::HybridLogGamma;
		} break;
		case VK_COLOR_SPACE_HDR10_HLG_EXT: {
			_mode = ColorCorrectionMode::HybridLogGamma;
		} break;
		case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT: {
			_mode = ColorCorrectionMode::PassThrough; // TODO check
		} break;
		case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT: {
			_mode = ColorCorrectionMode::AdobeRGB;
		} break;
		case VK_COLOR_SPACE_PASS_THROUGH_EXT: {
			_mode = ColorCorrectionMode::PassThrough; // TODO check
		} break;
		case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT: {
			_mode = ColorCorrectionMode::scRGB;
			_gamma = 1.0 / 2.4;
		} break;
		case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD: {
			_mode = ColorCorrectionMode::PassThrough; // TODO check
		} break;
		}
		return res;
	}
}