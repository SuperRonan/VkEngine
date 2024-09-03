#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/Core/DynamicValue.hpp>

#include <that/utils/EnumClassOperators.hpp>

#include <vkl/Maths/Types.hpp>

namespace vkl
{
	// Reminder on Vulkan Blending terminology:
	// the source value is the one produced by the fragment shader
	// the destination value is the one already present in the framebuffer

	// Same layout as VkPipelineColorBlendAttachmentState
	struct VkPipelineColorBlendAttachmentStateVKL
	{
		VkBool32 enable;
		VkColorBlendEquationEXT equation;
		VkColorComponentFlags write_mask;
	};

	constexpr static VkPipelineColorBlendAttachmentState Cast(VkPipelineColorBlendAttachmentStateVKL const& v)
	{
		return VkPipelineColorBlendAttachmentState{
			.blendEnable = v.enable,
			.srcColorBlendFactor = v.equation.srcColorBlendFactor,
			.dstColorBlendFactor = v.equation.dstColorBlendFactor,
			.colorBlendOp = v.equation.colorBlendOp,
			.srcAlphaBlendFactor = v.equation.srcAlphaBlendFactor,
			.dstAlphaBlendFactor = v.equation.dstAlphaBlendFactor,
			.alphaBlendOp = v.equation.alphaBlendOp,
			.colorWriteMask = v.write_mask,
		};
	}

	enum class BlendOp : uint8_t
	{
		Add = VK_BLEND_OP_ADD,
		Substract = VK_BLEND_OP_SUBTRACT,
		FragMinusFb = Substract,
		ReverseSubstract = VK_BLEND_OP_REVERSE_SUBTRACT,
		FbMinusFrag = ReverseSubstract,
		Min = VK_BLEND_OP_MIN,
		Max = VK_BLEND_OP_MAX,
		NUM_CORE_BLEND_OP = Max + 1,
		ZeroEXT					= NUM_CORE_BLEND_OP + 0,
		SrcEXT					= NUM_CORE_BLEND_OP + 1,
		DstEXT					= NUM_CORE_BLEND_OP + 2,
		SrcOverEXT				= NUM_CORE_BLEND_OP + 3,
		DstOverEXT				= NUM_CORE_BLEND_OP + 4,
		SrcInEXT				= NUM_CORE_BLEND_OP + 5,
		DstInEXT				= NUM_CORE_BLEND_OP + 6,
		SrcOutEXT				= NUM_CORE_BLEND_OP + 7,
		DstOutEXT				= NUM_CORE_BLEND_OP + 8,
		SrcAtopEXT				= NUM_CORE_BLEND_OP + 9,
		DstAtopEXT				= NUM_CORE_BLEND_OP + 10,
		XorEXT					= NUM_CORE_BLEND_OP + 11,
		MultiplyEXT				= NUM_CORE_BLEND_OP + 12,
		ScreenEXT				= NUM_CORE_BLEND_OP + 13,
		OverlayEXT				= NUM_CORE_BLEND_OP + 14,
		DarkenEXT				= NUM_CORE_BLEND_OP + 15,
		LightenEXT				= NUM_CORE_BLEND_OP + 16,
		ColorDodgeEXT			= NUM_CORE_BLEND_OP + 17,
		ColorBurnEXT			= NUM_CORE_BLEND_OP + 18,
		HardLightEXT			= NUM_CORE_BLEND_OP + 19,
		SoftLightEXT			= NUM_CORE_BLEND_OP + 20,
		DifferenceEXT			= NUM_CORE_BLEND_OP + 21,
		ExclusionEXT			= NUM_CORE_BLEND_OP + 22,
		InvertEXT				= NUM_CORE_BLEND_OP + 23,
		InvertRGB_EXT			= NUM_CORE_BLEND_OP + 24,
		LinearDodgeEXT			= NUM_CORE_BLEND_OP + 25,
		LinearBurnEXT			= NUM_CORE_BLEND_OP + 26,
		VividLightEXT			= NUM_CORE_BLEND_OP + 27,
		LinearLightEXT			= NUM_CORE_BLEND_OP + 28,
		PanLightEXT				= NUM_CORE_BLEND_OP + 29,
		MardMixEXT				= NUM_CORE_BLEND_OP + 30,
		HSL_HUE_EXT				= NUM_CORE_BLEND_OP + 31,
		HSL_SaturationEXT		= NUM_CORE_BLEND_OP + 32,
		HSL_ColorEXT			= NUM_CORE_BLEND_OP + 33,
		HSL_LuminosityEXT		= NUM_CORE_BLEND_OP + 34,
		PlusEXT					= NUM_CORE_BLEND_OP + 35,
		PlusClampedEXT			= NUM_CORE_BLEND_OP + 36,
		PlusClampedAlphaEXT		= NUM_CORE_BLEND_OP + 37,
		PlusDarkerEXT			= NUM_CORE_BLEND_OP + 38,
		MinusEXT				= NUM_CORE_BLEND_OP + 39,
		MinusClampedEXT			= NUM_CORE_BLEND_OP + 40,
		ContrastEXT				= NUM_CORE_BLEND_OP + 41,
		Invert_OVG_EXT			= NUM_CORE_BLEND_OP + 42,
		RedEXT					= NUM_CORE_BLEND_OP + 43,
		GreenEXT				= NUM_CORE_BLEND_OP + 44,
		BlueEXT					= NUM_CORE_BLEND_OP + 45,
		MAX_VALUE_INCLUDED = BlueEXT,
	};
	THAT_DECLARE_ENUM_CLASS_OPERATORS(BlendOp)

	enum class BlendFactor : uint8_t
	{
		Zero					= VK_BLEND_FACTOR_ZERO,
		One						= VK_BLEND_FACTOR_ONE,
		SrcColor				= VK_BLEND_FACTOR_SRC_COLOR,
		OneMinusSrcColor		= VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		DstColor				= VK_BLEND_FACTOR_DST_COLOR,
		OneMinusDstColor		= VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		SrcAlpha				= VK_BLEND_FACTOR_SRC_ALPHA,
		OneMinusSrcAlpha		= VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		DstAlpha				= VK_BLEND_FACTOR_DST_ALPHA,
		OneMinusDstAlpha		= VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		ConstantColor			= VK_BLEND_FACTOR_CONSTANT_COLOR,
		OneMinusConstantColor	= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
		ConstantAlpha			= VK_BLEND_FACTOR_CONSTANT_ALPHA,
		OneMinusConstantAlpha	= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
		SrcAlphaSaturate		= VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
		Src1Color				= VK_BLEND_FACTOR_SRC1_COLOR,
		OneMinusSrc1Color		= VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
		Src1Alpha				= VK_BLEND_FACTOR_SRC1_ALPHA,
		OneMinusSrc1Alpha		= VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
		MAX_VALUE_INCLUDED		= OneMinusSrc1Alpha,
	};
	THAT_DECLARE_ENUM_CLASS_OPERATORS(BlendFactor)

	namespace impl
	{
		using AttachmentBlendingFlagsUint = uint64_t;

		static constexpr const AttachmentBlendingFlagsUint BlendOpMinBits = std::bit_width<AttachmentBlendingFlagsUint>(static_cast<AttachmentBlendingFlagsUint>(BlendOp::MAX_VALUE_INCLUDED));
		static constexpr const AttachmentBlendingFlagsUint BlendFactorMinBits = std::bit_width<AttachmentBlendingFlagsUint>(static_cast<AttachmentBlendingFlagsUint>(BlendFactor::MAX_VALUE_INCLUDED));

		static constexpr const AttachmentBlendingFlagsUint BlendWriteMaskOffset = 1;
		static constexpr const AttachmentBlendingFlagsUint BlendWriteMaskBits = 4;

		static constexpr const AttachmentBlendingFlagsUint SrcColorBlendFactorOffset = BlendWriteMaskOffset + BlendWriteMaskBits;
		static constexpr const AttachmentBlendingFlagsUint DstColorBlendFactorOffset = SrcColorBlendFactorOffset + BlendFactorMinBits;
		static constexpr const AttachmentBlendingFlagsUint ColorBlendOpOffset = DstColorBlendFactorOffset + BlendFactorMinBits;

		static constexpr const AttachmentBlendingFlagsUint SrcAlphaBlendFactorOffset = ColorBlendOpOffset + BlendOpMinBits;
		static constexpr const AttachmentBlendingFlagsUint DstAlphaBlendFactorOffset = SrcAlphaBlendFactorOffset + BlendFactorMinBits;
		static constexpr const AttachmentBlendingFlagsUint AlphaBlendOpOffset = DstAlphaBlendFactorOffset + BlendFactorMinBits;


		struct AttachmentBlendFlagsAccessor {};
	}

	struct ColorBlendOp : public impl::AttachmentBlendFlagsAccessor
	{
		BlendOp value;

		constexpr ColorBlendOp(BlendOp op) :
			value(op)
		{}

		static constexpr const auto BitOffset = impl::ColorBlendOpOffset;
		static constexpr const auto BitCount = impl::BlendOpMinBits;
	};

	struct AlphaBlendOp : public impl::AttachmentBlendFlagsAccessor
	{
		BlendOp value;

		constexpr AlphaBlendOp(BlendOp op) :
			value(op)
		{}

		static constexpr const auto BitOffset = impl::AlphaBlendOpOffset;
		static constexpr const auto BitCount = impl::BlendOpMinBits;
	};

	struct SrcColorBlendFactor : public impl::AttachmentBlendFlagsAccessor
	{
		BlendFactor value;

		constexpr SrcColorBlendFactor(BlendFactor f) :
			value(f)
		{}

		static constexpr const auto BitOffset = impl::SrcColorBlendFactorOffset;
		static constexpr const auto BitCount = impl::BlendFactorMinBits;
	};

	struct DstColorBlendFactor : public impl::AttachmentBlendFlagsAccessor
	{
		BlendFactor value;

		constexpr DstColorBlendFactor(BlendFactor f) :
			value(f)
		{}

		static constexpr const auto BitOffset = impl::DstColorBlendFactorOffset;
		static constexpr const auto BitCount = impl::BlendFactorMinBits;
	};

	struct SrcAlphaBlendFactor : public impl::AttachmentBlendFlagsAccessor
	{
		BlendFactor value;

		constexpr SrcAlphaBlendFactor(BlendFactor f) :
			value(f)
		{}

		static constexpr const auto BitOffset = impl::SrcAlphaBlendFactorOffset;
		static constexpr const auto BitCount = impl::BlendFactorMinBits;
	};

	struct DstAlphaBlendFactor : public impl::AttachmentBlendFlagsAccessor
	{
		BlendFactor value;

		constexpr DstAlphaBlendFactor(BlendFactor f) :
			value(f)
		{}

		static constexpr const auto BitOffset = impl::DstAlphaBlendFactorOffset;
		static constexpr const auto BitCount = impl::BlendFactorMinBits;
	};

	static constexpr VkBlendOp Convert(BlendOp op)
	{
		VkBlendOp res;
		if (op < BlendOp::NUM_CORE_BLEND_OP)
		{
			res = static_cast<VkBlendOp>(op);
		}
		else
		{
			res = static_cast<VkBlendOp>(static_cast<int>(op - BlendOp::ZeroEXT) + VK_BLEND_OP_ZERO_EXT);
		}
		return res;
	}

	static constexpr BlendOp Convert(VkBlendOp op)
	{
		BlendOp res;
		if (op <= VK_BLEND_OP_MAX)
		{
			res = static_cast<BlendOp>(op);
		}
		else
		{
			res = static_cast<BlendOp>(op - VK_BLEND_OP_ZERO_EXT) + BlendOp::ZeroEXT;
		}
		return res;
	}

	static constexpr VkBlendFactor Convert(BlendFactor f)
	{
		return static_cast<VkBlendFactor>(f);
	}

	static constexpr BlendFactor Convert(VkBlendFactor f)
	{
		return static_cast<BlendFactor>(f);
	}

	namespace impl
	{
		enum class AttachmentBlendingFlags : impl::AttachmentBlendingFlagsUint
		{
			None = 0,

			DisableBlending = 0x0,
			EnableBlending = 0x1,

			WriteRed = static_cast<impl::AttachmentBlendingFlagsUint>(VK_COLOR_COMPONENT_R_BIT) << impl::BlendWriteMaskOffset,
			WriteGreen = static_cast<impl::AttachmentBlendingFlagsUint>(VK_COLOR_COMPONENT_G_BIT) << impl::BlendWriteMaskOffset,
			WriteBlue = static_cast<impl::AttachmentBlendingFlagsUint>(VK_COLOR_COMPONENT_B_BIT) << impl::BlendWriteMaskOffset,
			WriteAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(VK_COLOR_COMPONENT_A_BIT) << impl::BlendWriteMaskOffset,
			WriteRGB = static_cast<impl::AttachmentBlendingFlagsUint>(VK_COLOR_COMPONENTS_RGB_BITS) << impl::BlendWriteMaskOffset,
			WriteRGBA = static_cast<impl::AttachmentBlendingFlagsUint>(VK_COLOR_COMPONENTS_RGBA_BITS) << impl::BlendWriteMaskOffset,
			WriteMask = WriteRGBA,

			SrcColorFactorZero = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Zero) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOne = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::One) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorSrcColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcColor) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOneMinusSrcColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrcColor) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorDstColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::DstColor) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOneMinusDstColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusDstColor) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorSrcAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcAlpha) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOneMinusSrcAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrcAlpha) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorDstAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::DstAlpha) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOneMinusDstAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusDstAlpha) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorConstantColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::ConstantColor) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOneMinusConstantColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusConstantColor) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorConstantAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::ConstantAlpha) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOneMinusConstantAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusConstantAlpha) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorSrcAlphaSaturate = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcAlphaSaturate) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorSrc1Color = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Src1Color) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOneMinusSrc1Color = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrc1Color) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorSrc1Alpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Src1Alpha) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorOneMinusSrc1Alpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrc1Alpha) << impl::SrcColorBlendFactorOffset,
			SrcColorFactorMask = std::bitMask(impl::BlendFactorMinBits) << impl::SrcColorBlendFactorOffset,

			DstColorFactorZero = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Zero) << impl::DstColorBlendFactorOffset,
			DstColorFactorOne = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::One) << impl::DstColorBlendFactorOffset,
			DstColorFactorSrcColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcColor) << impl::DstColorBlendFactorOffset,
			DstColorFactorOneMinusSrcColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrcColor) << impl::DstColorBlendFactorOffset,
			DstColorFactorDstColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::DstColor) << impl::DstColorBlendFactorOffset,
			DstColorFactorOneMinusDstColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusDstColor) << impl::DstColorBlendFactorOffset,
			DstColorFactorSrcAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcAlpha) << impl::DstColorBlendFactorOffset,
			DstColorFactorOneMinusSrcAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrcAlpha) << impl::DstColorBlendFactorOffset,
			DstColorFactorDstAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::DstAlpha) << impl::DstColorBlendFactorOffset,
			DstColorFactorOneMinusDstAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusDstAlpha) << impl::DstColorBlendFactorOffset,
			DstColorFactorConstantColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::ConstantColor) << impl::DstColorBlendFactorOffset,
			DstColorFactorOneMinusConstantColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusConstantColor) << impl::DstColorBlendFactorOffset,
			DstColorFactorConstantAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::ConstantAlpha) << impl::DstColorBlendFactorOffset,
			DstColorFactorOneMinusConstantAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusConstantAlpha) << impl::DstColorBlendFactorOffset,
			DstColorFactorSrcAlphaSaturate = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcAlphaSaturate) << impl::DstColorBlendFactorOffset,
			DstColorFactorSrc1Color = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Src1Color) << impl::DstColorBlendFactorOffset,
			DstColorFactorOneMinusSrc1Color = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrc1Color) << impl::DstColorBlendFactorOffset,
			DstColorFactorSrc1Alpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Src1Alpha) << impl::DstColorBlendFactorOffset,
			DstColorFactorOneMinusSrc1Alpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrc1Alpha) << impl::DstColorBlendFactorOffset,
			DstColorFactorMask = std::bitMask(impl::BlendFactorMinBits) << impl::DstColorBlendFactorOffset,

			ColorOpAdd = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Add) << impl::ColorBlendOpOffset,
			ColorOpSubstract = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Substract) << impl::ColorBlendOpOffset,
			ColorOpFragMinusFb = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::FragMinusFb) << impl::ColorBlendOpOffset,
			ColorOpReverseSubstract = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ReverseSubstract) << impl::ColorBlendOpOffset,
			ColorOpFbMinusFrag = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::FbMinusFrag) << impl::ColorBlendOpOffset,
			ColorOpMin = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Min) << impl::ColorBlendOpOffset,
			ColorOpMax = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Max) << impl::ColorBlendOpOffset,
			ColorOpZeroEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ZeroEXT) << impl::ColorBlendOpOffset,
			ColorOpSrcEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcEXT) << impl::ColorBlendOpOffset,
			ColorOpDstEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstEXT) << impl::ColorBlendOpOffset,
			ColorOpSrcOverEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcOverEXT) << impl::ColorBlendOpOffset,
			ColorOpDstOverEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstOverEXT) << impl::ColorBlendOpOffset,
			ColorOpSrcInEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcInEXT) << impl::ColorBlendOpOffset,
			ColorOpDstInEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstInEXT) << impl::ColorBlendOpOffset,
			ColorOpSrcOutEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcOutEXT) << impl::ColorBlendOpOffset,
			ColorOpDstOutEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstOutEXT) << impl::ColorBlendOpOffset,
			ColorOpSrcAtopEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcAtopEXT) << impl::ColorBlendOpOffset,
			ColorOpDstAtopEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstAtopEXT) << impl::ColorBlendOpOffset,
			ColorOpXorEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::XorEXT) << impl::ColorBlendOpOffset,
			ColorOpMultiplyEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::MultiplyEXT) << impl::ColorBlendOpOffset,
			ColorOpScreenEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ScreenEXT) << impl::ColorBlendOpOffset,
			ColorOpOverlayEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::OverlayEXT) << impl::ColorBlendOpOffset,
			ColorOpDarkenEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DarkenEXT) << impl::ColorBlendOpOffset,
			ColorOpLightenEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::LightenEXT) << impl::ColorBlendOpOffset,
			ColorOpColorDodgeEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ColorDodgeEXT) << impl::ColorBlendOpOffset,
			ColorOpColorBurnEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ColorBurnEXT) << impl::ColorBlendOpOffset,
			ColorOpHardLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HardLightEXT) << impl::ColorBlendOpOffset,
			ColorOpSoftLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SoftLightEXT) << impl::ColorBlendOpOffset,
			ColorOpDifferenceEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DifferenceEXT) << impl::ColorBlendOpOffset,
			ColorOpExclusionEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ExclusionEXT) << impl::ColorBlendOpOffset,
			ColorOpInvertEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::InvertEXT) << impl::ColorBlendOpOffset,
			ColorOpInvertRGB_EXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::InvertRGB_EXT) << impl::ColorBlendOpOffset,
			ColorOpLinearDodgeEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::LinearDodgeEXT) << impl::ColorBlendOpOffset,
			ColorOpLinearBurnEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::LinearBurnEXT) << impl::ColorBlendOpOffset,
			ColorOpVividLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::VividLightEXT) << impl::ColorBlendOpOffset,
			ColorOpLinearLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::LinearLightEXT) << impl::ColorBlendOpOffset,
			ColorOpPanLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PanLightEXT) << impl::ColorBlendOpOffset,
			ColorOpMardMixEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::MardMixEXT) << impl::ColorBlendOpOffset,
			ColorOpHSL_HUE_EXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HSL_HUE_EXT) << impl::ColorBlendOpOffset,
			ColorOpHSL_SaturationEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HSL_SaturationEXT) << impl::ColorBlendOpOffset,
			ColorOpHSL_ColorEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HSL_ColorEXT) << impl::ColorBlendOpOffset,
			ColorOpHSL_LuminosityEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HSL_LuminosityEXT) << impl::ColorBlendOpOffset,
			ColorOpPlusEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PlusEXT) << impl::ColorBlendOpOffset,
			ColorOpPlusClampedEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PlusClampedEXT) << impl::ColorBlendOpOffset,
			ColorOpPlusClampedAlphaEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PlusClampedAlphaEXT) << impl::ColorBlendOpOffset,
			ColorOpPlusDarkerEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PlusDarkerEXT) << impl::ColorBlendOpOffset,
			ColorOpMinusEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::MinusEXT) << impl::ColorBlendOpOffset,
			ColorOpMinusClampedEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::MinusClampedEXT) << impl::ColorBlendOpOffset,
			ColorOpContrastEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ContrastEXT) << impl::ColorBlendOpOffset,
			ColorOpInvert_OVG_EXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Invert_OVG_EXT) << impl::ColorBlendOpOffset,
			ColorOpRedEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::RedEXT) << impl::ColorBlendOpOffset,
			ColorOpGreenEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::GreenEXT) << impl::ColorBlendOpOffset,
			ColorOpBlueEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::BlueEXT) << impl::ColorBlendOpOffset,
			ColorOpMask = std::bitMask(impl::BlendOpMinBits) << impl::ColorBlendOpOffset,


			SrcAlphaFactorZero = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Zero) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOne = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::One) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorSrcColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcColor) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOneMinusSrcColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrcColor) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorDstColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::DstColor) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOneMinusDstColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusDstColor) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorSrcAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcAlpha) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOneMinusSrcAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrcAlpha) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorDstAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::DstAlpha) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOneMinusDstAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusDstAlpha) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorConstantColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::ConstantColor) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOneMinusConstantColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusConstantColor) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorConstantAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::ConstantAlpha) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOneMinusConstantAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusConstantAlpha) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorSrcAlphaSaturate = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcAlphaSaturate) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorSrc1Color = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Src1Color) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOneMinusSrc1Color = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrc1Color) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorSrc1Alpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Src1Alpha) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorOneMinusSrc1Alpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrc1Alpha) << impl::SrcAlphaBlendFactorOffset,
			SrcAlphaFactorMask = std::bitMask(impl::BlendFactorMinBits) << impl::SrcAlphaBlendFactorOffset,

			DstAlphaFactorZero = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Zero) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOne = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::One) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorSrcColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcColor) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOneMinusSrcColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrcColor) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorDstColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::DstColor) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOneMinusDstColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusDstColor) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorSrcAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcAlpha) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOneMinusSrcAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrcAlpha) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorDstAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::DstAlpha) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOneMinusDstAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusDstAlpha) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorConstantColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::ConstantColor) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOneMinusConstantColor = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusConstantColor) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorConstantAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::ConstantAlpha) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOneMinusConstantAlpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusConstantAlpha) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorSrcAlphaSaturate = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::SrcAlphaSaturate) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorSrc1Color = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Src1Color) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOneMinusSrc1Color = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrc1Color) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorSrc1Alpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::Src1Alpha) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorOneMinusSrc1Alpha = static_cast<impl::AttachmentBlendingFlagsUint>(BlendFactor::OneMinusSrc1Alpha) << impl::DstAlphaBlendFactorOffset,
			DstAlphaFactorMask = std::bitMask(impl::BlendFactorMinBits) << impl::DstAlphaBlendFactorOffset,

			AlphaOpAdd = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Add) << impl::AlphaBlendOpOffset,
			AlphaOpSubstract = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Substract) << impl::AlphaBlendOpOffset,
			AlphaOpFragMinusFb = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::FragMinusFb) << impl::AlphaBlendOpOffset,
			AlphaOpReverseSubstract = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ReverseSubstract) << impl::AlphaBlendOpOffset,
			AlphaOpFbMinusFrag = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::FbMinusFrag) << impl::AlphaBlendOpOffset,
			AlphaOpMin = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Min) << impl::AlphaBlendOpOffset,
			AlphaOpMax = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Max) << impl::AlphaBlendOpOffset,
			AlphaOpZeroEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ZeroEXT) << impl::AlphaBlendOpOffset,
			AlphaOpSrcEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcEXT) << impl::AlphaBlendOpOffset,
			AlphaOpDstEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstEXT) << impl::AlphaBlendOpOffset,
			AlphaOpSrcOverEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcOverEXT) << impl::AlphaBlendOpOffset,
			AlphaOpDstOverEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstOverEXT) << impl::AlphaBlendOpOffset,
			AlphaOpSrcInEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcInEXT) << impl::AlphaBlendOpOffset,
			AlphaOpDstInEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstInEXT) << impl::AlphaBlendOpOffset,
			AlphaOpSrcOutEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcOutEXT) << impl::AlphaBlendOpOffset,
			AlphaOpDstOutEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstOutEXT) << impl::AlphaBlendOpOffset,
			AlphaOpSrcAtopEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SrcAtopEXT) << impl::AlphaBlendOpOffset,
			AlphaOpDstAtopEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DstAtopEXT) << impl::AlphaBlendOpOffset,
			AlphaOpXorEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::XorEXT) << impl::AlphaBlendOpOffset,
			AlphaOpMultiplyEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::MultiplyEXT) << impl::AlphaBlendOpOffset,
			AlphaOpScreenEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ScreenEXT) << impl::AlphaBlendOpOffset,
			AlphaOpOverlayEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::OverlayEXT) << impl::AlphaBlendOpOffset,
			AlphaOpDarkenEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DarkenEXT) << impl::AlphaBlendOpOffset,
			AlphaOpLightenEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::LightenEXT) << impl::AlphaBlendOpOffset,
			AlphaOpColorDodgeEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ColorDodgeEXT) << impl::AlphaBlendOpOffset,
			AlphaOpColorBurnEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ColorBurnEXT) << impl::AlphaBlendOpOffset,
			AlphaOpHardLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HardLightEXT) << impl::AlphaBlendOpOffset,
			AlphaOpSoftLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::SoftLightEXT) << impl::AlphaBlendOpOffset,
			AlphaOpDifferenceEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::DifferenceEXT) << impl::AlphaBlendOpOffset,
			AlphaOpExclusionEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ExclusionEXT) << impl::AlphaBlendOpOffset,
			AlphaOpInvertEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::InvertEXT) << impl::AlphaBlendOpOffset,
			AlphaOpInvertRGB_EXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::InvertRGB_EXT) << impl::AlphaBlendOpOffset,
			AlphaOpLinearDodgeEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::LinearDodgeEXT) << impl::AlphaBlendOpOffset,
			AlphaOpLinearBurnEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::LinearBurnEXT) << impl::AlphaBlendOpOffset,
			AlphaOpVividLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::VividLightEXT) << impl::AlphaBlendOpOffset,
			AlphaOpLinearLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::LinearLightEXT) << impl::AlphaBlendOpOffset,
			AlphaOpPanLightEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PanLightEXT) << impl::AlphaBlendOpOffset,
			AlphaOpMardMixEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::MardMixEXT) << impl::AlphaBlendOpOffset,
			AlphaOpHSL_HUE_EXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HSL_HUE_EXT) << impl::AlphaBlendOpOffset,
			AlphaOpHSL_SaturationEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HSL_SaturationEXT) << impl::AlphaBlendOpOffset,
			AlphaOpHSL_ColorEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HSL_ColorEXT) << impl::AlphaBlendOpOffset,
			AlphaOpHSL_LuminosityEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::HSL_LuminosityEXT) << impl::AlphaBlendOpOffset,
			AlphaOpPlusEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PlusEXT) << impl::AlphaBlendOpOffset,
			AlphaOpPlusClampedEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PlusClampedEXT) << impl::AlphaBlendOpOffset,
			AlphaOpPlusClampedAlphaEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PlusClampedAlphaEXT) << impl::AlphaBlendOpOffset,
			AlphaOpPlusDarkerEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::PlusDarkerEXT) << impl::AlphaBlendOpOffset,
			AlphaOpMinusEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::MinusEXT) << impl::AlphaBlendOpOffset,
			AlphaOpMinusClampedEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::MinusClampedEXT) << impl::AlphaBlendOpOffset,
			AlphaOpContrastEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::ContrastEXT) << impl::AlphaBlendOpOffset,
			AlphaOpInvert_OVG_EXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::Invert_OVG_EXT) << impl::AlphaBlendOpOffset,
			AlphaOpRedEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::RedEXT) << impl::AlphaBlendOpOffset,
			AlphaOpGreenEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::GreenEXT) << impl::AlphaBlendOpOffset,
			AlphaOpBlueEXT = static_cast<impl::AttachmentBlendingFlagsUint>(BlendOp::BlueEXT) << impl::AlphaBlendOpOffset,
			AlphaOpMask = std::bitMask(impl::BlendOpMinBits) << impl::AlphaBlendOpOffset,

			BasicAlphaBlending = SrcColorFactorSrcAlpha | DstColorFactorOneMinusSrcAlpha | ColorOpAdd | SrcAlphaFactorOne | DstAlphaFactorZero | AlphaOpAdd,
		};
	}
	
	THAT_DECLARE_ENUM_CLASS_OPERATORS(impl::AttachmentBlendingFlags)
	THAT_DECLARE_ENUM_CLASS_BIT_FIELD_OPERATORS(impl::AttachmentBlendingFlags, impl::AttachmentBlendFlagsAccessor)


	struct AttachmentBlending
	{
		using FlagsUint = impl::AttachmentBlendingFlagsUint;

		using Flags = impl::AttachmentBlendingFlags;

		struct DetailedFlags
		{
			FlagsUint enable : 1;
			FlagsUint write_mask : impl::BlendWriteMaskBits;
			FlagsUint src_color_blend_factor : impl::BlendFactorMinBits;
			FlagsUint dst_color_blend_factor : impl::BlendFactorMinBits;
			FlagsUint color_blend_op : impl::BlendOpMinBits;
			FlagsUint src_alpha_blend_factor : impl::BlendFactorMinBits;
			FlagsUint dst_alpha_blend_factor : impl::BlendFactorMinBits;
			FlagsUint alpha_blend_op : impl::BlendOpMinBits;
		};

		union
		{
			Flags flags = Flags::DisableBlending;
			DetailedFlags detailed;
		};

		static constexpr AttachmentBlending DefaultOverwrite(VkColorComponentFlags write_mask = VK_COLOR_COMPONENTS_RGBA_BITS)
		{
			AttachmentBlending res;
			res |= Flags::DisableBlending;
			res |= write_mask;
			return res;
		}

		static inline Dyn<AttachmentBlending> DefaultOverwrite(Dyn<VkColorComponentFlags> write_mask)
		{
			if (write_mask)
			{
				return [write_mask]()
				{
					AttachmentBlending res;
					res |= Flags::DisableBlending;
					res |= write_mask.value();
					return res;
				};
			}
			else
			{
				return DefaultOverwrite();
			}
		}

		static constexpr AttachmentBlending DefaultAlphaBlending(VkColorComponentFlags write_mask = VK_COLOR_COMPONENTS_RGBA_BITS)
		{
			AttachmentBlending res;
			res |= Flags::EnableBlending;
			res |= write_mask;
			res |= Flags::BasicAlphaBlending;
			return res;
		}

		static inline Dyn<AttachmentBlending> DefaultAlphaBlending(Dyn<VkColorComponentFlags> write_mask)
		{
			if (write_mask)
			{
				return [write_mask]()
				{
					AttachmentBlending res;
					res |= Flags::EnableBlending;
					res |= write_mask.value();
					res |= Flags::BasicAlphaBlending;
					return res;
				};
			}
			else
			{
				return DefaultAlphaBlending();
			}
		}

		constexpr AttachmentBlending() = default;

		constexpr AttachmentBlending(Flags f):
			flags(f)
		{}

		constexpr AttachmentBlending(DetailedFlags d):
			detailed(d)
		{}

		constexpr AttachmentBlending& operator=(bool enable)
		{
			detailed.enable = enable ? 1 : 0;
			return *this;
		}

		constexpr AttachmentBlending& operator=(VkColorComponentFlags write_mask)
		{
			detailed.write_mask = write_mask;
			return *this;
		}

		constexpr AttachmentBlending& operator|= (VkColorComponentFlags write_mask)
		{
			detailed.write_mask |= write_mask;
			return *this;
		}

		constexpr AttachmentBlending& operator=(Flags f)
		{
			flags = f;
			return *this;
		}

		#define BLENDING_TMP_DECLARE_OP(op) \
		constexpr AttachmentBlending& operator ## op ## =(Flags f) \
		{ \
			flags op ## = f; \
			return *this; \
		} \
		constexpr AttachmentBlending operator ## op(Flags f) \
		{ \
			AttachmentBlending res = *this; \
			res op ## = f; \
			return res; \
		}

		BLENDING_TMP_DECLARE_OP(|)
		BLENDING_TMP_DECLARE_OP(&)
		BLENDING_TMP_DECLARE_OP(^)

		#undef BLENDING_TMP_DECLARE_OP

		template <std::derived_from<impl::AttachmentBlendFlagsAccessor> Accessor>
		constexpr AttachmentBlending& operator=(Accessor a)
		{
			flags &= ~(std::bitMask<FlagsUint>(Accessor::BitCount) << Accessor::BitOffset);
			flags |= a;
			return *this;
		}

		template <std::derived_from<impl::AttachmentBlendFlagsAccessor> Accessor>
		constexpr AttachmentBlending& operator|=(Accessor a)
		{
			flags |= a;
			return *this;
		}

		template <std::derived_from<impl::AttachmentBlendFlagsAccessor> Accessor>
		constexpr AttachmentBlending operator|(Accessor a) const
		{
			AttachmentBlending res = *this;
			res |= a;
			return res;
		}

		constexpr bool isEnabled()const
		{
			return !!(flags & Flags::EnableBlending);
		}

		constexpr VkColorComponentFlags extractWriteMask() const
		{
			VkColorComponentFlags res = static_cast<VkColorComponentFlags>((flags >> impl::BlendWriteMaskOffset) & std::bitMask(impl::BlendWriteMaskBits));
			return res;
		}

		constexpr VkColorBlendEquationEXT extractBlendEquation() const
		{
			VkColorBlendEquationEXT res{
				.srcColorBlendFactor = Convert(static_cast<BlendFactor>(detailed.src_color_blend_factor)),
				.dstColorBlendFactor = Convert(static_cast<BlendFactor>(detailed.dst_color_blend_factor)),
				.colorBlendOp = Convert(static_cast<BlendOp>(detailed.color_blend_op)),
				.srcAlphaBlendFactor = Convert(static_cast<BlendFactor>(detailed.src_alpha_blend_factor)),
				.dstAlphaBlendFactor = Convert(static_cast<BlendFactor>(detailed.dst_alpha_blend_factor)),
				.alphaBlendOp = Convert(static_cast<BlendOp>(detailed.alpha_blend_op)),
			};
			return res;
		}

		constexpr VkPipelineColorBlendAttachmentStateVKL extract() const
		{
			VkPipelineColorBlendAttachmentStateVKL res{
				.enable = detailed.enable ? VK_TRUE : VK_FALSE,
				.equation = extractBlendEquation(),
				.write_mask = extractWriteMask(),
			};
			return res;
		}

		static constexpr Flags Reduce(Flags f)
		{
			const constexpr Flags mask = static_cast<Flags>(std::bitMask<FlagsUint>(impl::BlendWriteMaskBits + 1));
			if (!(f & Flags::EnableBlending))
			{
				f &= mask;
			}
			return f;
		}

		static constexpr bool Equivalent(AttachmentBlending l, AttachmentBlending r)
		{
			return Reduce(l.flags) == Reduce(r.flags);
		}

		constexpr operator VkPipelineColorBlendAttachmentState() const
		{
			return Cast(extract());
		}

		static constexpr AttachmentBlending MakeFromFast(VkPipelineColorBlendAttachmentStateVKL const& v)
		{
			AttachmentBlending res;
			Flags & flags = res.flags;
			flags |= (v.enable);
			flags |= (v.write_mask << impl::BlendWriteMaskOffset);
			res |= SrcColorBlendFactor(Convert(v.equation.srcColorBlendFactor));
			res |= DstColorBlendFactor(Convert(v.equation.dstColorBlendFactor));
			res |= ColorBlendOp(Convert(v.equation.colorBlendOp));
			res |= SrcAlphaBlendFactor(Convert(v.equation.srcAlphaBlendFactor));
			res |= SrcAlphaBlendFactor(Convert(v.equation.dstAlphaBlendFactor));
			res |= AlphaBlendOp(Convert(v.equation.alphaBlendOp));
			return res;
		}

		constexpr bool usesConstantBlendFactor() const
		{
			bool res = false;
			if (!!(flags & Flags::EnableBlending))
			{
				constexpr const bool exclude_ext_blend_ops = true;
				constexpr const auto is_constant = [](impl::AttachmentBlendingFlagsUint fi)
				{
					const BlendFactor f = static_cast<BlendFactor>(fi);
					return (f >= BlendFactor::ConstantColor) && (f <= BlendFactor::OneMinusConstantAlpha);
				};
				if (!exclude_ext_blend_ops && static_cast<BlendOp>(detailed.color_blend_op) < BlendOp::NUM_CORE_BLEND_OP)
				{
					res |= is_constant(detailed.src_color_blend_factor);
					res |= is_constant(detailed.dst_color_blend_factor);
				}
				if (!exclude_ext_blend_ops && static_cast<BlendOp>(detailed.alpha_blend_op) < BlendOp::NUM_CORE_BLEND_OP)
				{
					res |= is_constant(detailed.src_alpha_blend_factor);
					res |= is_constant(detailed.dst_alpha_blend_factor);
				}
			}
			return res;
		}
	};

	enum class LogicOp : uint8_t
	{
		Clear = VK_LOGIC_OP_CLEAR,
		And = VK_LOGIC_OP_AND,
		SrcAndNotDst = VK_LOGIC_OP_AND_REVERSE,
		FragAndNotFb = SrcAndNotDst,
		PassThrough = VK_LOGIC_OP_COPY,
		NotSrcAndDst = VK_LOGIC_OP_AND_INVERTED,
		NotFragAndFb = NotSrcAndDst,
		NoOp = VK_LOGIC_OP_NO_OP,
		Xor = VK_LOGIC_OP_XOR,
		Or = VK_LOGIC_OP_OR,
		Nor = VK_LOGIC_OP_NOR,
		Equivalent = VK_LOGIC_OP_EQUIVALENT,
		NotDst = VK_LOGIC_OP_INVERT,
		NotFb = NotDst,
		SrcOrNotDst = VK_LOGIC_OP_OR_REVERSE,
		FragOrNotFb = SrcOrNotDst,
		NotSrc = VK_LOGIC_OP_COPY_INVERTED,
		NotFrag = NotSrc,
		NotSrcOrDst = VK_LOGIC_OP_OR_INVERTED,
		NotFragOrFb = NotSrcOrDst,
		Nand = VK_LOGIC_OP_NAND,
		Set = VK_LOGIC_OP_SET,
		MAX_VALUE_INCLUDED = Set,
	};

	static constexpr LogicOp Convert(VkLogicOp lop)
	{
		return static_cast<LogicOp>(lop);
	}

	static constexpr VkLogicOp Convert(LogicOp lop)
	{
		return static_cast<VkLogicOp>(lop);
	}

	namespace impl
	{	
		using PipelineBlendFlagsUint = uint32_t;

		const constexpr PipelineBlendFlagsUint PipelineBlendCoreFlagsBitOffset = 0;
		const constexpr PipelineBlendFlagsUint PipelineBlendCoreFlagsBitCount = 1;

		const constexpr PipelineBlendFlagsUint UseLogicOpBitOffset = PipelineBlendCoreFlagsBitOffset + PipelineBlendCoreFlagsBitCount;

		const constexpr PipelineBlendFlagsUint LogicOpBitOffset = UseLogicOpBitOffset + 1;
		const constexpr PipelineBlendFlagsUint LogicOpBitCount = std::bit_width<PipelineBlendFlagsUint>(static_cast<PipelineBlendFlagsUint>(LogicOp::MAX_VALUE_INCLUDED));

		const constexpr PipelineBlendFlagsUint UseAdvancedBlendingBitOffset = LogicOpBitOffset + LogicOpBitCount;
		
		const constexpr PipelineBlendFlagsUint SrcPremultipliedBitOffset = UseAdvancedBlendingBitOffset + 1;
		const constexpr PipelineBlendFlagsUint DstPremultipliedBitOffset = SrcPremultipliedBitOffset + 1;

		const constexpr PipelineBlendFlagsUint BlendOverlapBitOffset = DstPremultipliedBitOffset + 1;
		const constexpr PipelineBlendFlagsUint BlendOverlapBitCount = 2;
	

		enum class PipelineBlendingFlags : impl::PipelineBlendFlagsUint
		{
			None = 0x0,
			RasterizationOrderAttachmentAccess = VK_PIPELINE_COLOR_BLEND_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_BIT_EXT << impl::PipelineBlendCoreFlagsBitOffset,

			UseLogicOp = 0x1 << impl::UseLogicOpBitOffset,

			Clear = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::Clear) << impl::LogicOpBitOffset,
			And = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::And) << impl::LogicOpBitOffset,
			SrcAndNotDst = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::SrcAndNotDst) << impl::LogicOpBitOffset,
			FragAndNotFb = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::FragAndNotFb) << impl::LogicOpBitOffset,
			PassThrough = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::PassThrough) << impl::LogicOpBitOffset,
			NotSrcAndDst = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NotSrcAndDst) << impl::LogicOpBitOffset,
			NotFragAndFb = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NotFragAndFb) << impl::LogicOpBitOffset,
			NoOp = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NoOp) << impl::LogicOpBitOffset,
			Xor = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::Xor) << impl::LogicOpBitOffset,
			Or = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::Or) << impl::LogicOpBitOffset,
			Nor = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::Nor) << impl::LogicOpBitOffset,
			Equivalent = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::Equivalent) << impl::LogicOpBitOffset,
			NotDst = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NotDst) << impl::LogicOpBitOffset,
			NotFb = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NotFb) << impl::LogicOpBitOffset,
			SrcOrNotDst = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::SrcOrNotDst) << impl::LogicOpBitOffset,
			FragOrNotFb = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::FragOrNotFb) << impl::LogicOpBitOffset,
			NotSrc = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NotSrc) << impl::LogicOpBitOffset,
			NotFrag = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NotFrag) << impl::LogicOpBitOffset,
			NotSrcOrDst = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NotSrcOrDst) << impl::LogicOpBitOffset,
			NotFragOrFb = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::NotFragOrFb) << impl::LogicOpBitOffset,
			Nand = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::Nand) << impl::LogicOpBitOffset,
			Set = static_cast<impl::PipelineBlendFlagsUint>(LogicOp::Set) << impl::LogicOpBitOffset,

			UseAdvancedBlending = 0x1 << impl::UseAdvancedBlendingBitOffset,
		
			SrcPremultiplied = 0x1 << impl::SrcPremultipliedBitOffset,
			FragPremultiplied = SrcPremultiplied,
			DstPremultiplied = 0x1 << impl::DstPremultipliedBitOffset,
			FbPremultiplied = DstPremultiplied,

			BlendOverlapUncorrelated = VK_BLEND_OVERLAP_UNCORRELATED_EXT << impl::BlendOverlapBitOffset,
			BlendOverlapDisjoint = VK_BLEND_OVERLAP_DISJOINT_EXT << impl::BlendOverlapBitOffset,
			BlendOverlapConjoint = VK_BLEND_OVERLAP_CONJOINT_EXT << impl::BlendOverlapBitOffset,
		};

	}

	THAT_DECLARE_ENUM_CLASS_OPERATORS(impl::PipelineBlendingFlags)

	struct VkPipelineBlendingState
	{
		VkPipelineColorBlendStateCreateInfo ci;
		// advanced.sType is correct -> use advanced
		VkPipelineColorBlendAdvancedStateCreateInfoEXT advanced;

		constexpr bool useAdvanced() const
		{
			return advanced.sType == VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT;
		}

		void clear();

		VkPipelineColorBlendStateCreateInfo& link(const VkPipelineColorBlendAttachmentState * color_attachments, uint32_t count);
	};

	struct PipelineBlending
	{
		using Flags = impl::PipelineBlendingFlags;

		struct Detailed
		{
			impl::PipelineBlendFlagsUint core_flags : impl::PipelineBlendCoreFlagsBitCount;
			impl::PipelineBlendFlagsUint use_logic_op : 1;
			impl::PipelineBlendFlagsUint logic_op : impl::LogicOpBitCount;
			impl::PipelineBlendFlagsUint use_advanced_blending : 1;
			impl::PipelineBlendFlagsUint src_premultiplied : 1;
			impl::PipelineBlendFlagsUint dst_premultiplied : 1;
			impl::PipelineBlendFlagsUint blend_overlap : impl::BlendOverlapBitCount;
		};

		union
		{
			Flags flags = Flags::None;
			Detailed detailed;
		};

		Vector4f blend_constants = {};

		static PipelineBlending deduce(VkPipelineBlendingState const& s);

		void extract(VkPipelineBlendingState& dst) const;

		static constexpr Flags Reduce(Flags f)
		{
			if (!(f & Flags::UseLogicOp))
			{
				f &= (~std::bitMask<impl::PipelineBlendFlagsUint>(impl::LogicOpBitCount) << impl::LogicOpBitOffset);
			}
			if (!(f & Flags::UseAdvancedBlending))
			{
				f &= (~std::bitMask<impl::PipelineBlendFlagsUint>(1 + 1 + impl::BlendOverlapBitCount) << impl::SrcPremultipliedBitOffset);
			}
			return f;
		}

		static constexpr bool Equivalent(PipelineBlending const& l, PipelineBlending const& r, bool consider_blend_constants)
		{
			bool res = (Reduce(l.flags) == Reduce(r.flags));
			if (res && consider_blend_constants)
			{
				res &= (l.blend_constants == r.blend_constants);
			}
			return res;
		}
	};
}

