#include "DetailedVkFormat.hpp"
#include <functional>

namespace vkl
{
	static_assert(DetailedVkFormat::Type::UNORM == img::ElementType::UNORM);
	static_assert(DetailedVkFormat::Type::SNORM == img::ElementType::SNORM);
	static_assert(DetailedVkFormat::Type::UINT == img::ElementType::UINT);
	static_assert(DetailedVkFormat::Type::SINT == img::ElementType::SINT);
	static_assert(DetailedVkFormat::Type::SRGB == img::ElementType::sRGB);
	static_assert(DetailedVkFormat::Type::SFLOAT == img::ElementType::FLOAT);

	std::vector<DetailedVkFormat> DetailedVkFormat::s_table = {
		DetailedVkFormat(VK_FORMAT_UNDEFINED),
		DetailedVkFormat(VK_FORMAT_R4G4_UNORM_PACK8, Type::UNORM, 2, 4, Swizzle::RG, 8),
		DetailedVkFormat(VK_FORMAT_R4G4B4A4_UNORM_PACK16, Type::UNORM, 4, 4, Swizzle::RGBA, 16),
		DetailedVkFormat(VK_FORMAT_B4G4R4A4_UNORM_PACK16, Type::UNORM, 4, 4, Swizzle::BGRA, 16),
		DetailedVkFormat(VK_FORMAT_R5G6B5_UNORM_PACK16, Type::UNORM, 3, {5, 6, 5, 0}, Swizzle::RGB, 16),
		DetailedVkFormat(VK_FORMAT_B5G6R5_UNORM_PACK16, Type::UNORM, 3, {5, 6, 5, 0}, Swizzle::BGR, 16),
		DetailedVkFormat(VK_FORMAT_R5G5B5A1_UNORM_PACK16, Type::UNORM, 4, {5, 5, 5, 1}, Swizzle::RGBA, 16),
		DetailedVkFormat(VK_FORMAT_B5G5R5A1_UNORM_PACK16, Type::UNORM, 4, {5, 5, 5, 1}, Swizzle::BGRA, 16),
		DetailedVkFormat(VK_FORMAT_A1R5G5B5_UNORM_PACK16, Type::UNORM, 4, {1, 5, 5, 5}, Swizzle::ARGB, 16),
		DetailedVkFormat(VK_FORMAT_R8_UNORM, Type::UNORM, 1, 8, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R8_SNORM, Type::SNORM, 1, 8, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R8_USCALED, Type::USCALED, 1, 8, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R8_SSCALED, Type::SSCALED, 1, 8, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R8_UINT, Type::UINT, 1, 8, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R8_SINT, Type::SINT, 1, 8, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R8_SRGB, Type::SRGB, 1, 8, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R8G8_UNORM, Type::UNORM, 2, 8, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R8G8_SNORM, Type::SNORM, 2, 8, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R8G8_USCALED, Type::USCALED, 2, 8, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R8G8_SSCALED, Type::SSCALED, 2, 8, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R8G8_UINT, Type::UINT, 2, 8, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R8G8_SINT, Type::SINT, 2, 8, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R8G8_SRGB, Type::SRGB, 2, 8, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8_UNORM, Type::UNORM, 3, 8, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8_SNORM, Type::SNORM, 3, 8, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8_USCALED, Type::USCALED, 3, 8, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8_SSCALED, Type::SSCALED, 3, 8, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8_UINT, Type::UINT, 3, 8, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8_SINT, Type::SINT, 3, 8, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8_SRGB, Type::SRGB, 3, 8, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8_UNORM, Type::UNORM, 3, 8, Swizzle::BGR, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8_SNORM, Type::SNORM, 3, 8, Swizzle::BGR, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8_USCALED, Type::USCALED, 3, 8, Swizzle::BGR, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8_SSCALED, Type::SSCALED, 3, 8, Swizzle::BGR, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8_UINT, Type::UINT, 3, 8, Swizzle::BGR, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8_SINT, Type::SINT, 3, 8, Swizzle::BGR, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8_SRGB, Type::SRGB, 3, 8, Swizzle::BGR, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8A8_UNORM, Type::UNORM, 4, 8, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8A8_SNORM, Type::SNORM, 4, 8, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8A8_USCALED, Type::USCALED, 4, 8, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8A8_SSCALED, Type::SSCALED, 4, 8, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8A8_UINT, Type::UINT, 4, 8, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8A8_SINT, Type::SINT, 4, 8, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R8G8B8A8_SRGB, Type::SRGB, 4, 8, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8A8_UNORM, Type::UNORM, 4, 8, Swizzle::BGRA, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8A8_SNORM, Type::SNORM, 4, 8, Swizzle::BGRA, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8A8_USCALED, Type::USCALED, 4, 8, Swizzle::BGRA, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8A8_SSCALED, Type::SSCALED, 4, 8, Swizzle::BGRA, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8A8_UINT, Type::UINT, 4, 8, Swizzle::BGRA, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8A8_SINT, Type::SINT, 4, 8, Swizzle::BGRA, 0),
		DetailedVkFormat(VK_FORMAT_B8G8R8A8_SRGB, Type::SRGB, 4, 8, Swizzle::BGRA, 0),
		DetailedVkFormat(VK_FORMAT_A8B8G8R8_UNORM_PACK32, Type::UNORM, 4, 8, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A8B8G8R8_SNORM_PACK32, Type::SNORM, 4, 8, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A8B8G8R8_USCALED_PACK32, Type::USCALED, 4, 8, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A8B8G8R8_SSCALED_PACK32, Type::SSCALED, 4, 8, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A8B8G8R8_UINT_PACK32, Type::UINT, 4, 8, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A8B8G8R8_SINT_PACK32, Type::SINT, 4, 8, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A8B8G8R8_SRGB_PACK32, Type::SRGB, 4, 8, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A2R10G10B10_UNORM_PACK32, Type::UNORM, 4, {2, 10, 10, 10}, Swizzle::ARGB, 32),
		DetailedVkFormat(VK_FORMAT_A2R10G10B10_SNORM_PACK32, Type::SNORM, 4, {2, 10, 10, 10}, Swizzle::ARGB, 32),
		DetailedVkFormat(VK_FORMAT_A2R10G10B10_USCALED_PACK32, Type::USCALED, 4, {2, 10, 10, 10}, Swizzle::ARGB, 32),
		DetailedVkFormat(VK_FORMAT_A2R10G10B10_SSCALED_PACK32, Type::SSCALED, 4, {2, 10, 10, 10}, Swizzle::ARGB, 32),
		DetailedVkFormat(VK_FORMAT_A2R10G10B10_UINT_PACK32, Type::UINT, 4, {2, 10, 10, 10}, Swizzle::ARGB, 32),
		DetailedVkFormat(VK_FORMAT_A2R10G10B10_SINT_PACK32, Type::SINT, 4, {2, 10, 10, 10}, Swizzle::ARGB, 32),
		DetailedVkFormat(VK_FORMAT_A2B10G10R10_UNORM_PACK32, Type::UNORM, 4, {2, 10, 10, 10}, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A2B10G10R10_SNORM_PACK32, Type::SNORM, 4, {2, 10, 10, 10}, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A2B10G10R10_USCALED_PACK32, Type::USCALED, 4, {2, 10, 10, 10}, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A2B10G10R10_SSCALED_PACK32, Type::SSCALED, 4, {2, 10, 10, 10}, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A2B10G10R10_UINT_PACK32, Type::UINT, 4, {2, 10, 10, 10}, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_A2B10G10R10_SINT_PACK32, Type::SINT, 4, {2, 10, 10, 10}, Swizzle::ABGR, 32),
		DetailedVkFormat(VK_FORMAT_R16_UNORM, Type::UNORM, 1, 16, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R16_SNORM, Type::SNORM, 1, 16, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R16_USCALED, Type::USCALED, 1, 16, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R16_SSCALED, Type::SSCALED, 1, 16, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R16_UINT, Type::UINT, 1, 16, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R16_SINT, Type::SINT, 1, 16, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R16_SFLOAT, Type::SFLOAT, 1, 16, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R16G16_UNORM, Type::UNORM, 2, 16, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R16G16_SNORM, Type::SNORM, 2, 16, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R16G16_USCALED, Type::USCALED, 2, 16, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R16G16_SSCALED, Type::SSCALED, 2, 16, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R16G16_UINT, Type::UINT, 2, 16, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R16G16_SINT, Type::SINT, 2, 16, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R16G16_SFLOAT, Type::SFLOAT, 2, 16, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16_UNORM, Type::UNORM, 3, 16, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16_SNORM, Type::SNORM, 3, 16, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16_USCALED, Type::USCALED, 3, 16, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16_SSCALED, Type::SSCALED, 3, 16, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16_UINT, Type::UINT, 3, 16, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16_SINT, Type::SINT, 3, 16, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16_SFLOAT, Type::SFLOAT, 3, 16, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16A16_UNORM, Type::UNORM, 4, 16, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16A16_SNORM, Type::SNORM, 4, 16, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16A16_USCALED, Type::USCALED, 4, 16, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16A16_SSCALED, Type::SSCALED, 4, 16, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16A16_UINT, Type::UINT, 4, 16, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16A16_SINT, Type::SINT, 4, 16, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R16G16B16A16_SFLOAT, Type::SFLOAT, 4, 16, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R32_UINT, Type::UINT, 1, 32, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R32_SINT, Type::SINT, 1, 32, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R32_SFLOAT, Type::SFLOAT, 1, 32, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R32G32_UINT, Type::UINT, 2, 32, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R32G32_SINT, Type::SINT, 2, 32, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R32G32_SFLOAT, Type::SFLOAT, 2, 32, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R32G32B32_UINT, Type::UINT, 3, 32, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R32G32B32_SINT, Type::SINT, 3, 32, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R32G32B32_SFLOAT, Type::SFLOAT, 3, 32, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R32G32B32A32_UINT, Type::UINT, 4, 32, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R32G32B32A32_SINT, Type::SINT, 4, 32, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R32G32B32A32_SFLOAT, Type::SFLOAT, 4, 32, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R64_UINT, Type::UINT, 1, 64, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R64_SINT, Type::SINT, 1, 64, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R64_SFLOAT, Type::SFLOAT, 1, 64, Swizzle::R, 0),
		DetailedVkFormat(VK_FORMAT_R64G64_UINT, Type::UINT, 2, 64, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R64G64_SINT, Type::SINT, 2, 64, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R64G64_SFLOAT, Type::SFLOAT, 2, 64, Swizzle::RG, 0),
		DetailedVkFormat(VK_FORMAT_R64G64B64_UINT, Type::UINT, 3, 64, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R64G64B64_SINT, Type::SINT, 3, 64, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R64G64B64_SFLOAT, Type::SFLOAT, 3, 64, Swizzle::RGB, 0),
		DetailedVkFormat(VK_FORMAT_R64G64B64A64_UINT, Type::UINT, 4, 64, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R64G64B64A64_SINT, Type::SINT, 4, 64, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_R64G64B64A64_SFLOAT, Type::SFLOAT, 4, 64, Swizzle::RGBA, 0),
		DetailedVkFormat(VK_FORMAT_B10G11R11_UFLOAT_PACK32, Type::UFLOAT, 3, {10, 11, 11, 0}, Swizzle::BGR, 32),
		DetailedVkFormat(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, Type::UFLOAT, 3, {16, 16, 16, 0}, Swizzle::EBGR, 32),
		DetailedVkFormat(VK_FORMAT_D16_UNORM, Type::UNORM, 16, Type::None, 0, 0),
		DetailedVkFormat(VK_FORMAT_X8_D24_UNORM_PACK32, Type::UNORM, 24, Type::None, 0, 32),
		DetailedVkFormat(VK_FORMAT_D32_SFLOAT, Type::SFLOAT, 32, Type::None, 0, 0),
		DetailedVkFormat(VK_FORMAT_S8_UINT, Type::None, 0, Type::UINT, 8, 0),
		DetailedVkFormat(VK_FORMAT_D16_UNORM_S8_UINT, Type::UNORM, 16, Type::UINT, 8, 0),
		DetailedVkFormat(VK_FORMAT_D24_UNORM_S8_UINT, Type::UNORM, 24, Type::UINT, 8, 0),
		DetailedVkFormat(VK_FORMAT_D32_SFLOAT_S8_UINT, Type::SFLOAT, 32, Type::UINT, 8, 0),
		DetailedVkFormat(VK_FORMAT_BC1_RGB_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC1_RGB_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC1_RGBA_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC1_RGBA_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC2_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC2_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC3_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC3_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC4_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC4_SNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC5_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC5_SNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC6H_UFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC6H_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC7_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_BC7_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_EAC_R11_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_EAC_R11_SNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_EAC_R11G11_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_EAC_R11G11_SNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_4x4_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_4x4_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_5x4_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_5x4_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_5x5_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_5x5_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_6x5_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_6x5_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_6x6_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_6x6_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x5_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x5_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x6_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x6_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x8_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x8_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x5_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x5_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x6_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x6_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x8_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x8_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x10_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x10_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_12x10_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_12x10_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_12x12_UNORM_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_12x12_SRGB_BLOCK),
		DetailedVkFormat(VK_FORMAT_G8B8G8R8_422_UNORM),
		DetailedVkFormat(VK_FORMAT_B8G8R8G8_422_UNORM),
		DetailedVkFormat(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM),
		DetailedVkFormat(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM),
		DetailedVkFormat(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM),
		DetailedVkFormat(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM),
		DetailedVkFormat(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM),
		DetailedVkFormat(VK_FORMAT_R10X6_UNORM_PACK16),
		DetailedVkFormat(VK_FORMAT_R10X6G10X6_UNORM_2PACK16),
		DetailedVkFormat(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16),
		DetailedVkFormat(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16),
		DetailedVkFormat(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16),
		DetailedVkFormat(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_R12X4_UNORM_PACK16),
		DetailedVkFormat(VK_FORMAT_R12X4G12X4_UNORM_2PACK16),
		DetailedVkFormat(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16),
		DetailedVkFormat(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16),
		DetailedVkFormat(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16),
		DetailedVkFormat(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G16B16G16R16_422_UNORM),
		DetailedVkFormat(VK_FORMAT_B16G16R16G16_422_UNORM),
		DetailedVkFormat(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM),
		DetailedVkFormat(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM),
		DetailedVkFormat(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM),
		DetailedVkFormat(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM),
		DetailedVkFormat(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM),
		DetailedVkFormat(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM),
		DetailedVkFormat(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16),
		DetailedVkFormat(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM),
		DetailedVkFormat(VK_FORMAT_A4R4G4B4_UNORM_PACK16),
		DetailedVkFormat(VK_FORMAT_A4B4G4R4_UNORM_PACK16),
		DetailedVkFormat(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK),
		DetailedVkFormat(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG),
		DetailedVkFormat(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG),
		DetailedVkFormat(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG),
		DetailedVkFormat(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG),
		DetailedVkFormat(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG),
		DetailedVkFormat(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG),
		DetailedVkFormat(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG),
		DetailedVkFormat(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG),
		DetailedVkFormat(VK_FORMAT_R16G16_S10_5_NV),
		DetailedVkFormat(VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR),
		DetailedVkFormat(VK_FORMAT_A8_UNORM_KHR),
		DetailedVkFormat(VK_FORMAT_MAX_ENUM),
	};

	DetailedVkFormat::ReverseMap DetailedVkFormat::s_reversed_map = DetailedVkFormat::makeReversedMap();

	size_t DetailedVkFormat::ColorFormatDetailedInfo::hash()const
	{
		std::hash<size_t> hs;
		std::hash<uint32_t> hu;
		size_t res = hu(type) ^ hu(channels) ^ hu(swizzle);
		size_t hb = 0;
		for (uint32_t i = 0; i < channels; ++i)
		{
			hb ^= hu(bits[i]);
		}
		res = res ^ hs(hb);
		return res;
	}

	size_t DetailedVkFormat::DepthStencilFormatDetailedInfo::hash()const
	{
		std::hash<size_t> hst;
		std::hash<uint32_t> hu;
		size_t hd = hu(depth_type) ^ hu(depth_bits);
		size_t hs = hu(stencil_type) ^ hu(stencil_bits);
		size_t res = hst(hd ^ hs);
		return res;
	}

	size_t DetailedVkFormat::Hasher::operator()(DetailedVkFormat const& f) const
	{
		size_t res = 0;
		std::hash<size_t> hs;
		std::hash<uint32_t> hu;
		res = hu(f.aspect) ^ hu(f.pack_bits);
		if (f.aspect & VK_IMAGE_ASPECT_COLOR_BIT)
		{
			res ^= f.color.hash();
		}
		else if (f.aspect & VK_IMAGE_ASPECT_DEPTH_STENCIL_BITS)
		{
			res ^= f.depth_stencil.hash();
		}
		return res;
	}

	bool DetailedVkFormat::Equal::operator()(DetailedVkFormat const& f, DetailedVkFormat const& g) const
	{
		bool res = true;

		res = (f.aspect == g.aspect) && (f.pack_bits == g.pack_bits);
		if (res)
		{
			if (f.aspect & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				res = f.color == g.color;	
			}
			else if (f.aspect & VK_IMAGE_ASPECT_DEPTH_STENCIL_BITS)
			{
				res = f.depth_stencil == g.depth_stencil;
			}
		}
		return res;
	}

	DetailedVkFormat::ReverseMap DetailedVkFormat::makeReversedMap()
	{
		DetailedVkFormat::ReverseMap res;
		for (size_t i = 0; i < s_table.size(); ++i)
		{
			const DetailedVkFormat & f = s_table[i];
			if (f.aspect != VK_IMAGE_ASPECT_NONE)
			{
				assert(!res.contains(f));
				res[f] = f.vk_format;
			}
		}
		return res;
	}

	DetailedVkFormat::DetailedVkFormat(VkFormat f, Type type, uint32_t channels, std::array<uint32_t, 4> bits_per_component, Swizzle swizzle, uint32_t pack) :
		vk_format(f),
		aspect(VK_IMAGE_ASPECT_COLOR_BIT),
		color({
			.type = type,
			.channels = channels,
			.bits = bits_per_component,
			.swizzle = swizzle,
		}),
		pack_bits(pack)
	{}

	DetailedVkFormat::DetailedVkFormat(VkFormat f, Type type, uint32_t channels, uint32_t bits_per_component, Swizzle swizzle, uint32_t pack) :
		vk_format(f),
		aspect(VK_IMAGE_ASPECT_COLOR_BIT),
		color({
			.type = type,
			.channels = channels,
			.bits = {bits_per_component, bits_per_component, bits_per_component, bits_per_component},
			.swizzle = swizzle,
		}),
		pack_bits(pack)
	{
		for (uint32_t i = channels; i < 4; ++i)
		{
			color.bits[i] = 0;
		}
	}

	DetailedVkFormat::DetailedVkFormat(VkFormat f, ColorFormatDetailedInfo const& color_info, uint32_t pack):
		vk_format(f),
		aspect(VK_IMAGE_ASPECT_COLOR_BIT),
		color(color_info),
		pack_bits(pack)
	{}

	DetailedVkFormat::DetailedVkFormat(VkFormat f, Type depth_type, uint32_t depth_bits, Type stencil_type, uint32_t stencil_bits, uint32_t pack) :
		vk_format(f),
		aspect((depth_bits ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) | (stencil_bits ? VK_IMAGE_ASPECT_STENCIL_BIT : 0)),
		depth_stencil({
			.depth_type = depth_type,
			.depth_bits = depth_bits,
			.stencil_type = stencil_type,
			.stencil_bits = stencil_bits,
		}),
		pack_bits(pack)
	{}

	DetailedVkFormat::DetailedVkFormat(VkFormat f, DepthStencilFormatDetailedInfo const& dsi, uint32_t pack) :
		vk_format(f),
		aspect((dsi.depth_bits ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) | (dsi.stencil_bits ? VK_IMAGE_ASPECT_STENCIL_BIT : 0)),
		depth_stencil(dsi),
		pack_bits(pack)
	{}

	DetailedVkFormat::DetailedVkFormat(img::FormatInfo const& f) : 
		aspect(VK_IMAGE_ASPECT_COLOR_BIT)
	{
		color = {};
		color.channels = f.channels;
		for (uint32_t i = 0; i < color.channels; ++i)
		{
			color.bits[i] = f.elem_size * 8;
		}
		color.type = static_cast<Type>(f.type);
	}

	bool DetailedVkFormat::determineVkFormatFromInfo()
	{
		bool res = false;
		if (s_reversed_map.contains(*this))
		{
			vk_format = s_reversed_map.at(*this);
			res = true;
		}

		return res;
	}

	DetailedVkFormat DetailedVkFormat::Find(VkFormat f)
	{
		assert(checkVkFormatIsValid(f));
		DetailedVkFormat res = s_table.at(getContiguousIndexFromVkFormat(f));
		return res;
	}

	img::FormatInfo DetailedVkFormat::getImgFormatInfo()const
	{
		assert(aspect & VK_IMAGE_ASPECT_COLOR_BIT);
		img::FormatInfo res{
			.type = static_cast<img::ElementType>(color.type),
			.elem_size = color.bits[0] / 8,
			.channels = color.channels,
		};
		return res;
	}
}