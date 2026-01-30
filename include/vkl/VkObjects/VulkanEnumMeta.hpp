#pragma once

#include <format>

#include <vkl/Generated/VulkanEnumMeta.hpp>

namespace vku
{
	static constexpr const unsigned int VMAEnumIdBegin = EnumUniqueIdCount;

#define BEGIN_ENUM_META_INFO_ID(EnumTypeName, BitField, Id, Prefix) \
	template<> \
	struct EnumMetaInfo<EnumTypeName> \
	{ \
		static constexpr const bool IsBitField = BitField; \
		static constexpr const unsigned int UniqueId = Id; \
		using EnumType = EnumTypeName; \
		static constexpr const char* Name = X_STRINGIFY(EnumTypeName); \
		static constexpr const char* ValuePrefix = X_STRINGIFY(Prefix); \
		static constexpr const char* GetValueName(EnumTypeName value) \
		{ \
			const char* res = nullptr; \
			switch(value) \
			{
#define ENUM_META_INFO_CASE(Prefix, Suffix) case Prefix##_##Suffix: res = X_STRINGIFY(Suffix); break;
#define END_ENUM_META_INFO \
			} \
			return res; \
		} \
	};

#define BEGIN_ENUM_META_INFO(EnumTypeName, BitField, Prev, Prefix) BEGIN_ENUM_META_INFO_ID(EnumTypeName, BitField, EnumMetaInfo<Prev>::UniqueId + 1, Prefix)

	BEGIN_ENUM_META_INFO_ID(VmaAllocatorCreateFlagBits, true, VMAEnumIdBegin + 0, VMA_ALLOCATOR_CREATE)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, EXTERNALLY_SYNCHRONIZED_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, KHR_DEDICATED_ALLOCATION_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, KHR_BIND_MEMORY2_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, EXT_MEMORY_BUDGET_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, AMD_DEVICE_COHERENT_MEMORY_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, BUFFER_DEVICE_ADDRESS_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, EXT_MEMORY_PRIORITY_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, KHR_MAINTENANCE4_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATOR_CREATE, KHR_MAINTENANCE5_BIT)
	END_ENUM_META_INFO

	BEGIN_ENUM_META_INFO(VmaMemoryUsage, false, VmaAllocatorCreateFlagBits, VMA_MEMORY_USAGE)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, UNKNOWN)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, GPU_ONLY)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, CPU_ONLY)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, CPU_TO_GPU)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, GPU_TO_CPU)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, CPU_COPY)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, GPU_LAZILY_ALLOCATED)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, AUTO)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, AUTO_PREFER_DEVICE)
		ENUM_META_INFO_CASE(VMA_MEMORY_USAGE, AUTO_PREFER_HOST)
	END_ENUM_META_INFO

	BEGIN_ENUM_META_INFO(VmaAllocationCreateFlagBits, true, VmaMemoryUsage, VMA_ALLOCATION_CREATE)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, DEDICATED_MEMORY_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, NEVER_ALLOCATE_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, MAPPED_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, USER_DATA_COPY_STRING_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, UPPER_ADDRESS_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, DONT_BIND_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, WITHIN_BUDGET_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, CAN_ALIAS_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, HOST_ACCESS_RANDOM_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, STRATEGY_MIN_MEMORY_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, STRATEGY_MIN_TIME_BIT)
		ENUM_META_INFO_CASE(VMA_ALLOCATION_CREATE, STRATEGY_MIN_OFFSET_BIT)
	END_ENUM_META_INFO

	BEGIN_ENUM_META_INFO(VmaPoolCreateFlagBits, true, VmaAllocationCreateFlagBits, VMA_POOL_CREATE)
		ENUM_META_INFO_CASE(VMA_POOL_CREATE, IGNORE_BUFFER_IMAGE_GRANULARITY_BIT)
		ENUM_META_INFO_CASE(VMA_POOL_CREATE, LINEAR_ALGORITHM_BIT)
	END_ENUM_META_INFO

	BEGIN_ENUM_META_INFO(VmaDefragmentationFlagBits, true, VmaPoolCreateFlagBits, VMA_DEFRAGMENTATION_FLAG_ALGORITHM)
		ENUM_META_INFO_CASE(VMA_DEFRAGMENTATION_FLAG_ALGORITHM, FAST_BIT)
		ENUM_META_INFO_CASE(VMA_DEFRAGMENTATION_FLAG_ALGORITHM, BALANCED_BIT)
		ENUM_META_INFO_CASE(VMA_DEFRAGMENTATION_FLAG_ALGORITHM, FULL_BIT)
		ENUM_META_INFO_CASE(VMA_DEFRAGMENTATION_FLAG_ALGORITHM, EXTENSIVE_BIT)
	END_ENUM_META_INFO

	BEGIN_ENUM_META_INFO(VmaDefragmentationMoveOperation, false, VmaDefragmentationFlagBits, VMA_DEFRAGMENTATION_MOVE_OPERATION)
		ENUM_META_INFO_CASE(VMA_DEFRAGMENTATION_MOVE_OPERATION, COPY)
		ENUM_META_INFO_CASE(VMA_DEFRAGMENTATION_MOVE_OPERATION, IGNORE)
		ENUM_META_INFO_CASE(VMA_DEFRAGMENTATION_MOVE_OPERATION, DESTROY)
	END_ENUM_META_INFO

	BEGIN_ENUM_META_INFO(VmaVirtualBlockCreateFlagBits, true, VmaDefragmentationMoveOperation, VMA_VIRTUAL_BLOCK_CREATE)
		ENUM_META_INFO_CASE(VMA_VIRTUAL_BLOCK_CREATE, LINEAR_ALGORITHM_BIT)
	END_ENUM_META_INFO

	BEGIN_ENUM_META_INFO(VmaVirtualAllocationCreateFlagBits, true, VmaVirtualBlockCreateFlagBits, VMA_VIRTUAL_ALLOCATION_CREATE)
		ENUM_META_INFO_CASE(VMA_VIRTUAL_ALLOCATION_CREATE, UPPER_ADDRESS_BIT)
		ENUM_META_INFO_CASE(VMA_VIRTUAL_ALLOCATION_CREATE, STRATEGY_MIN_MEMORY_BIT)
		ENUM_META_INFO_CASE(VMA_VIRTUAL_ALLOCATION_CREATE, STRATEGY_MIN_TIME_BIT)
		ENUM_META_INFO_CASE(VMA_VIRTUAL_ALLOCATION_CREATE, STRATEGY_MIN_OFFSET_BIT)
	END_ENUM_META_INFO

	static constexpr const unsigned int VmaEnumIdCount = EnumMetaInfo<VmaVirtualAllocationCreateFlagBits>::UniqueId - VMAEnumIdBegin + 1;
}

namespace vkl
{
	namespace concepts
	{
		template <class E>
		concept VkEnum = that::concepts::Enumeration<E> && ::vku::EnumMetaInfo<E>::Name != nullptr;

		template <class E, bool BitField>
		concept VkEnum2 = VkEnum<E> && ::vku::EnumMetaInfo<E>::IsBitField == BitField;

		template <class E>
		concept VkRawEnum = VkEnum2<E, false>;

		template <class E>
		concept VkFlagBits = VkEnum2<E, true>;
	}

	// 32-bit FlagBits are int enum, so the underlying type must be promoted to uint32_t
	// 64-bit FlagBits are already backed by uint64_t
	template <concepts::VkFlagBits VkFlagBits>
	using VkFlagBitsUnderlying = std::make_unsigned<typename std::underlying_type<VkFlagBits>::type>;

	namespace vku
	{
		template <concepts::VkFlagBits FlagBits>
		std::string GetFlagsStr(typename VkFlagBitsUnderlying<FlagBits>::type flags)
		{
			using U = typename VkFlagBitsUnderlying<FlagBits>::type;
			using Meta = ::vku::EnumMetaInfo<FlagBits>;
			std::string res;
			if (flags)
			{
				constexpr const unsigned int count = sizeof(U) * 8;
				for (unsigned int i = 0; i < count; ++i)
				{
					U bit = (U(1) << U(i));
					if (flags & bit)
					{
						if (!res.empty())
						{
							res += " | ";
						}
						const char* label = Meta::GetValueName(FlagBits(bit));
						if (label)
						{
							res += label;
						}
						else
						{
							std::format_to(std::back_inserter(res), "Unknown_Flag{}", i);
						}
					}
				}
			}
			else
			{
				res = "NONE";
			}
			return res;
		}

		template <concepts::VkEnum E>
		const char* GetEnumLabel(E value, const char* default_label = "Unknown")
		{
			const char* res = ::vku::EnumMetaInfo<E>::GetValueName(value);
			return res ? res : default_label;
		}
	}
}