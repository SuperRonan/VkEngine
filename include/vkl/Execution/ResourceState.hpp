#pragma once

#include <vkl/Core/VulkanCommons.hpp>

namespace vkl
{
	struct ResourceState1
	{
		VkAccessFlags access = VK_ACCESS_NONE_KHR;
		VkPipelineStageFlags stage = VK_PIPELINE_STAGE_NONE_KHR;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	struct ResourceState2
	{
		VkAccessFlags2 access = VK_ACCESS_2_NONE_KHR;
		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE_KHR;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

		constexpr bool operator==(ResourceState2 const& o) const noexcept
		{
			return 
				(access == o.access) && 
				(layout == o.layout) &&
				(stage == o.stage)
			;
		}

		constexpr ResourceState2& operator|=(ResourceState2 const& o)
		{
			access |= o.access;
			stage |= o.stage;
			// Keep this layout
			return *this;
		}
		
		constexpr ResourceState2 operator| (ResourceState2 const& o) const 
		{
			ResourceState2 res = *this;
			res |= o;
			return res;
		}

		constexpr ResourceState2& operator&=(ResourceState2 const& o)
		{
			access &= o.access;
			stage &= o.stage;
			// Keep this layout
			return *this;
		}

		constexpr ResourceState2 operator& (ResourceState2 const& o) const
		{
			ResourceState2 res = *this;
			res &= o;
			return res;
		}

		constexpr static ResourceState2 Full()
		{
			return ResourceState2{
				.access = VkAccessFlags2(size_t(-1)),
				.stage = VkPipelineStageFlags2(size_t(-1)),
			};
		}
	};

	struct DoubleResourceState2
	{
		ResourceState2 write_state = {};
		ResourceState2 read_only_state = {};

		constexpr bool operator==(DoubleResourceState2 const& o) const noexcept
		{
			return write_state == o.write_state && read_only_state == o.read_only_state;
		}
	};

	struct DoubleDoubleResourceState2
	{
		DoubleResourceState2 additive;
		DoubleResourceState2 multiplicative;
	};

	constexpr bool accessIsWrite1(VkAccessFlags access)
	{
		return access & (
			VK_ACCESS_HOST_WRITE_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_SHADER_WRITE_BIT |
			VK_ACCESS_TRANSFER_WRITE_BIT |
			VK_ACCESS_MEMORY_WRITE_BIT
			);
	}

	constexpr bool accessIsWrite2(VkAccessFlags2 access)
	{
		return access & (
			VK_ACCESS_2_SHADER_WRITE_BIT |
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_2_TRANSFER_WRITE_BIT |
			VK_ACCESS_2_HOST_WRITE_BIT |
			VK_ACCESS_2_MEMORY_WRITE_BIT |
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT |
			VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR |
			VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
			VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT |
			VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV |
			VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
			VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_NV |
			VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT |
			VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV
		);
	}

	constexpr bool accessIsRead1(VkAccessFlags access)
	{
		return access & (
			VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
			VK_ACCESS_INDEX_READ_BIT |
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
			VK_ACCESS_UNIFORM_READ_BIT |
			VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
			VK_ACCESS_SHADER_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_TRANSFER_READ_BIT |
			VK_ACCESS_HOST_READ_BIT |
			VK_ACCESS_MEMORY_READ_BIT
			);
	}

	constexpr bool accessIsRead2(VkAccessFlags2 access)
	{
		return access & (
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT |
			VK_ACCESS_2_INDEX_READ_BIT |
			VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT |
			VK_ACCESS_2_UNIFORM_READ_BIT |
			VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_SHADER_READ_BIT |
			VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_TRANSFER_READ_BIT |
			VK_ACCESS_2_HOST_READ_BIT |
			VK_ACCESS_2_MEMORY_READ_BIT |
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
			VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
			VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR |
			VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |
			VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT |
			VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV |
			VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR |
			VK_ACCESS_2_SHADING_RATE_IMAGE_READ_BIT_NV |
			VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR |
			VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_NV |
			VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT |
			VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT |
			VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT |
			VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI |
			VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR |
			VK_ACCESS_2_MICROMAP_READ_BIT_EXT |
			VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV
		);
	}

	constexpr int getAccessNature2(VkAccessFlags2 access)
	{
		int r = accessIsRead2(access) ? 1 : 0;
		int w = accessIsWrite2(access) ? 1 : 0;
		return (r | (w << 1));
	}

	constexpr bool accessIsReadAndWrite1(VkAccessFlags access)
	{
		return accessIsRead1(access) && accessIsWrite1(access);
	}

	constexpr bool accessIsReadAndWrite2(VkAccessFlags2 access)
	{
		return accessIsRead2(access) && accessIsWrite2(access);
	}

	constexpr bool accessIsReadonly2(VkAccessFlags2 access)
	{
		return accessIsRead2(access) && !accessIsWrite2(access);
	}

	constexpr bool layoutTransitionRequired(ResourceState1 prev, ResourceState1 next)
	{
		return (prev.layout != next.layout);
	}

	constexpr bool layoutTransitionRequired(ResourceState2 prev, ResourceState2 next)
	{
		return (prev.layout != next.layout);
	}

	constexpr bool stateTransitionRequiresSynchronization1(ResourceState1 prev, ResourceState1 next, bool is_image)
	{
		const bool res =
			!(accessIsRead1(prev.access) && accessIsRead1(next.access)) // Assuming that !read = write
			|| (is_image && layoutTransitionRequired(prev, next));
		return res;
	}

	constexpr bool stateTransitionRequiresSynchronization2(ResourceState2 prev, ResourceState2 next, bool is_image)
	{
		const bool res =
			!(accessIsRead2(prev.access) && accessIsRead2(next.access)) // Assuming that !read = write
			|| (is_image && layoutTransitionRequired(prev, next));
		return res;
	}

	//class ResourceStateTracker
	//{
	//public:

	//	using BufferKey = typename BufferInstance::ResourceKey;
	//	using ImageKey = typename ImageViewInstance::ResourceKey;

	//	template<class K, class V, class Hasher = std::hash<K>, class Eq = std::equal_to<K>>
	//	using Map = std::unordered_map<K, V, Hasher, Eq>;

	//	class BufferStates
	//	{
	//	protected:

	//		using Range = typename BufferInstance::Range;

	//		BufferInstance* _buffer = nullptr;

	//		Map<Range, ResourceState2> _states;

	//	public:

	//		BufferStates(BufferInstance* buffer = nullptr);

	//		ResourceState2 getState(Range const& r);

	//		void setState(Range const& r, ResourceState2 const& state);

	//	};

	//	class ImageStates
	//	{
	//	protected:

	//		using Range = VkImageSubresourceRange;

	//		struct Hasher
	//		{
	//			size_t operator()(Range const& r) const
	//			{
	//				const std::hash<size_t> hs;
	//				const std::hash<uint32_t> hu;
	//				// don't consider the aspect in the hash since it should be the same
	//				return hu(r.baseMipLevel) ^ hu(r.levelCount) ^ hu(r.baseArrayLayer) ^ hu(r.layerCount);
	//			}
	//		};

	//		ImageInstance* _image = nullptr;

	//		Map<Range, ResourceState2, Hasher> _states;

	//	public:

	//		ImageStates(ImageInstance* image = nullptr);

	//		ResourceState2 getState(Range const& r);

	//		void setState(Range const& r, ResourceState2 const& state);

	//	};

	//protected:


	//	Map<size_t, ImageStates> _images;
	//	Map<size_t, BufferStates> _buffers;


	//public:

	//	ResourceStateTracker();

	//	void registerImage(ImageInstance* image);

	//	void registerBuffer(BufferInstance* buffer);

	//	void releaseImage(ImageInstance* image);

	//	void releaseBuffer(BufferInstance* buffer);

	//	ResourceState2 getImageState(ImageKey const& key);

	//	ResourceState2 getImageState(ImageViewInstance* view);

	//	ResourceState2 getBufferState(BufferKey const& key);

	//	void setImageState(ImageKey const& key, ResourceState2 const& state);

	//	void setImageState(ImageViewInstance* view, ResourceState2 const& state);

	//	void setBufferState(BufferKey const& key, ResourceState2 const& state);

	//};
}

// TODO make my own OptionalResourceState2 or std::optional<ResourceState2>
//namespace std
//{
//	using namespace vkl;
//
//	template <>
//	class optional<ResourceState2>
//	{
//
//	};
//}