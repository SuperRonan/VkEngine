#pragma once

#include <vulkan/vulkan_core.h>
#include <that/core/Range.hpp>

#include <vkl/Generated/VulkanFeaturesMetaConstants.hpp>

#include <algorithm>

namespace vkl::meta
{
	struct FeaturesMeta
	{
	protected:
		static constinit const size_t _g_expected_features_label_buffer_length = (features::MaxLabelLength + 1) * features::LabelCount;
	public:
		using Index = that::BigEnoughUInt<_g_expected_features_label_buffer_length>::type;
		using Range = ::that::Range<Index>;
	protected:
		static const char* const _g_features_labels_buffer;
		static const Index * const _g_features_metas; // By flat index
		static const VkStructureType * const _g_features_sTypes; // By flat index & sorted
		static const size_t _g_structs_count;
		static constinit const size_t _g_label_stride = features::MaxLabelLength + 1;
	public:
		Range range = {};

		const char* operator[](unsigned int index) const noexcept
		{
			return _g_features_labels_buffer + (range.begin + index) * _g_label_stride;
		}

		Index size() const
		{
			return range.len;
		}

		static Index Flatten_sType(VkStructureType sType)
		{
			const VkStructureType* end = _g_features_sTypes + _g_structs_count;
			const VkStructureType* it = std::lower_bound(_g_features_sTypes, end, sType);
			if (it == end)
			{
				return Index(-1);
			}
			if (*it == sType)
			{
				return Index(it - _g_features_sTypes);
			}
			return Index(-1);
		}

		static FeaturesMeta GetMetaAt(uint index)
		{
			if (index >= _g_structs_count)
			{
				return {};
			}
			Index begin = _g_features_metas[index];
			Index end = _g_features_metas[index + 1];
			return FeaturesMeta{.range = {.begin = begin, .len = Index(end - begin)}};
		}

		static FeaturesMeta GetMetaOf(VkStructureType sType)
		{
			return GetMetaAt(Flatten_sType(sType));
		}

		// include VulkanFeaturesMetaTemplate to use
		template <class T>
		static consteval uint GetVkFeaturesStructFlatIndex();

		template <class T>
		static FeaturesMeta GetVkFeaturesMeta()
		{
			return GetMetaAt(GetVkFeaturesStructFlatIndex<T>());
		}
	};
} // namespace vkl::meta
