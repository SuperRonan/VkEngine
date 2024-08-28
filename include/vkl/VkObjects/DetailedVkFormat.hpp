#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vector>
#include <array>
#include <that/img/Image.hpp>
#include <unordered_map>

namespace vkl
{
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

	public:

		VkFormat vk_format = VK_FORMAT_MAX_ENUM;
		enum class Type
		{
			None = -1,
			UNORM = that::ElementType::UNORM,
			SNORM = that::ElementType::SNORM,
			USCALED = 16,
			SSCALED = 17,
			UINT = that::ElementType::UINT,
			SINT = that::ElementType::SINT,
			SRGB = that::ElementType::sRGB,
			UFLOAT = 18,
			SFLOAT = that::ElementType::FLOAT,
			MAX_ENUM = 0x7f'ff'ff'ff,
		};
		enum class Swizzle
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
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_NONE;

		struct ColorFormatDetailedInfo
		{
			Type type;
			uint32_t channels;
			std::array<uint32_t, 4> bits;
			Swizzle swizzle;

			constexpr bool operator==(ColorFormatDetailedInfo const& o) const = default;

			size_t hash()const;
		};

		struct DepthStencilFormatDetailedInfo
		{
			Type depth_type;
			uint32_t depth_bits;
			Type stencil_type;
			uint32_t stencil_bits;

			constexpr bool operator==(DepthStencilFormatDetailedInfo const& o) const = default;

			size_t hash() const;
		};

		union
		{
			ColorFormatDetailedInfo color = {};
			DepthStencilFormatDetailedInfo depth_stencil;
		};

		// 0 means not packed
		uint32_t pack_bits = 0;

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
		DetailedVkFormat(VkFormat f, Type type, uint32_t channels, std::array<uint32_t, 4> bits_per_component, Swizzle swizzle, uint32_t pack);
		DetailedVkFormat(VkFormat f, Type type, uint32_t channels, uint32_t bits_per_component, Swizzle swizzle, uint32_t pack);
		DetailedVkFormat(VkFormat f, ColorFormatDetailedInfo const& color_info, uint32_t pack);

		// Depth Stencil Constructor
		DetailedVkFormat(VkFormat f, Type depth_type, uint32_t depth_bits, Type stencil_type, uint32_t stencil_bits, uint32_t pack);
		DetailedVkFormat(VkFormat f, DepthStencilFormatDetailedInfo const& depth_stencil_info, uint32_t pack);
		
		
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