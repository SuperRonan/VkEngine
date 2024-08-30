#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vector>
#include <array>
#include <that/img/Image.hpp>
#include <unordered_map>

namespace vkl
{

#ifndef VKL_USE_COMPRESSED_DETAILED_FORMAT
#define VKL_USE_COMPRESSED_DETAILED_FORMAT 1
#endif

	struct DetailedVkFormat
	{
	protected:

		static std::vector<DetailedVkFormat> s_table;

		struct Hasher
		{
			size_t operator()(DetailedVkFormat const& f) const;
		};

		struct Equal
		{
			bool operator()(DetailedVkFormat const& f, DetailedVkFormat const& g) const;
		};

		using ReverseMap = std::unordered_map<DetailedVkFormat, VkFormat, Hasher, Equal>;
		static ReverseMap s_reversed_map;

		static ReverseMap makeReversedMap();

		static constexpr uint32_t TypeCustomTypesOffset = static_cast<uint32_t>(that::ElementType::MAX_ENUM);
		

	public:

#if VKL_USE_COMPRESSED_DETAILED_FORMAT
		using PackBits_t = uint8_t;
		using BitCount_t = uint8_t;
		using ChannelCount_t = uint8_t;
		using TypeUint_t = uint8_t;
		using SwizzleUint_t = uint8_t;
#else
		using PackBits_t = uint32_t;
		using BitCount_t = uint32_t;
		using ChannelCount_t = uint32_t;
		using TypeUint_t = uint32_t;
		using SwizzleUint_t = uint32_t;
#endif

		VkFormat vk_format = VK_FORMAT_MAX_ENUM;
		enum class Type : TypeUint_t
		{
			None = TypeUint_t(~0u),
			UNORM = that::ElementType::UNORM,
			SNORM = that::ElementType::SNORM,
			USCALED = TypeCustomTypesOffset + 0,
			SSCALED = TypeCustomTypesOffset + 1,
			UINT = that::ElementType::UINT,
			SINT = that::ElementType::SINT,
			SRGB = that::ElementType::sRGB,
			UFLOAT = TypeCustomTypesOffset + 2,
			SFLOAT = that::ElementType::FLOAT,
			MAX_ENUM = TypeCustomTypesOffset + 3,
		};

		enum class Swizzle : SwizzleUint_t
		{
			RGBA = 0,
			RGB = RGBA,
			RG = RGB,
			R = RG,
			BGRA = 1,
			BGR = BGRA,
			ARGB = 2,
			ABGR = 3,
			EBGR = 4, // should it really exists separatly, or is it just a special RGB?
		};

	protected:
		
		static constexpr uint32_t MinTypeBits = std::bit_width(static_cast<uint32_t>(Type::MAX_ENUM));
		static constexpr uint32_t MinSwizzleBits = 3;
		static constexpr uint32_t MinAspectBits = std::max<uint32_t>(std::bit_width<uint32_t>(VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT), 16u);

	public:



#if VKL_USE_COMPRESSED_DETAILED_FORMAT
		uint16_t aspect : MinAspectBits = VK_IMAGE_ASPECT_NONE;
#else 
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_NONE;
#endif

		// 0 means not packed
		PackBits_t pack_bits = 0;

		struct ColorFormatDetailedInfo
		{
#if VKL_USE_COMPRESSED_DETAILED_FORMAT
			Type type : MinTypeBits;
			Swizzle swizzle : MinSwizzleBits;
			ChannelCount_t channels;
			std::array<BitCount_t, 4> bits;
#else
			Type type;
			Swizzle swizzle;
			ChannelCount_t  channels;
			std::array<BitCount_t, 4> bits;
#endif

			constexpr bool operator==(ColorFormatDetailedInfo const& o) const = default;

			size_t hash()const;
		};

		struct DepthStencilFormatDetailedInfo
		{
#if VKL_USE_COMPRESSED_DETAILED_FORMAT
			Type depth_type : MinTypeBits;
			Type stencil_type : MinTypeBits;
			BitCount_t depth_bits;
			BitCount_t stencil_bits;

#else
			Type depth_type;
			Type stencil_type;
			BitCount_t depth_bits;
			BitCount_t stencil_bits;
#endif

			constexpr bool operator==(DepthStencilFormatDetailedInfo const& o) const = default;

			size_t hash() const;
		};

		union
		{
			ColorFormatDetailedInfo color = {};
			DepthStencilFormatDetailedInfo depth_stencil;
		};

		DetailedVkFormat(DetailedVkFormat const& other) :
			vk_format(other.vk_format),
			aspect(other.aspect),
			pack_bits(other.pack_bits)
		{
			if (aspect & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				color = other.color;
			}
			if (aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
			{
				depth_stencil = other.depth_stencil;
			}
		}

		DetailedVkFormat& operator=(DetailedVkFormat const& other)
		{
			vk_format = other.vk_format;
			aspect = other.aspect;
			pack_bits = other.pack_bits;
			if (aspect & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				color = other.color;
			}
			if (aspect & VK_IMAGE_ASPECT_DEPTH_STENCIL_BITS)
			{
				depth_stencil = other.depth_stencil;
			}
			return *this;
		}


		constexpr DetailedVkFormat() = default;

		// Color constructor
		DetailedVkFormat(VkFormat f, Type type, ChannelCount_t channels, std::array<BitCount_t, 4> bits_per_component, Swizzle swizzle, PackBits_t pack);
		DetailedVkFormat(VkFormat f, Type type, ChannelCount_t channels, BitCount_t bits_per_component, Swizzle swizzle, PackBits_t pack);
		DetailedVkFormat(VkFormat f, ColorFormatDetailedInfo const& color_info, PackBits_t pack);

		// Depth Stencil Constructor
		DetailedVkFormat(VkFormat f, Type depth_type, BitCount_t depth_bits, Type stencil_type, BitCount_t stencil_bits, PackBits_t pack);
		DetailedVkFormat(VkFormat f, DepthStencilFormatDetailedInfo const& depth_stencil_info, PackBits_t pack);
		
		
		DetailedVkFormat(VkFormat f):
			vk_format(f)
		{}

		DetailedVkFormat(that::FormatInfo const& f);

		bool determineVkFormatFromInfo();

		static DetailedVkFormat Find(VkFormat format);

		that::FormatInfo getImgFormatInfo()const;

		std::string getGLSLName()const;

		VkColorComponentFlags getColorComponents() const;
	};
}