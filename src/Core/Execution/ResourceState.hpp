#pragma once

#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/VulkanCommons.hpp>

namespace vkl
{
	struct ResourceState1
	{
		VkAccessFlags _access = VK_ACCESS_NONE_KHR;
		VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkPipelineStageFlags _stage = VK_PIPELINE_STAGE_NONE_KHR;
	};

	struct ResourceState2
	{
		VkAccessFlags2 _access = VK_ACCESS_2_NONE_KHR;
		VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkPipelineStageFlags2 _stage = VK_PIPELINE_STAGE_2_NONE_KHR;
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
			VK_ACCESS_2_HOST_WRITE_BIT |
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_2_SHADER_WRITE_BIT |
			VK_ACCESS_2_TRANSFER_WRITE_BIT |
			VK_ACCESS_2_MEMORY_WRITE_BIT
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
			VK_ACCESS_2_MEMORY_READ_BIT
			);
	}

	constexpr bool accessIsReadAndWrite1(VkAccessFlags access)
	{
		return accessIsRead1(access) && accessIsWrite1(access);
	}

	constexpr bool accessIsReadAndWrite2(VkAccessFlags2 access)
	{
		return accessIsRead1(access) && accessIsWrite1(access);
	}

	constexpr bool layoutTransitionRequired(ResourceState1 prev, ResourceState1 next)
	{
		return (prev._layout != next._layout);
	}

	constexpr bool layoutTransitionRequired(ResourceState2 prev, ResourceState2 next)
	{
		return (prev._layout != next._layout);
	}

	constexpr bool stateTransitionRequiresSynchronization1(ResourceState1 prev, ResourceState1 next, bool is_image)
	{
		const bool res =
			!(accessIsRead1(prev._access) && accessIsRead1(next._access)) // Assuming that !read = write
			|| (is_image && layoutTransitionRequired(prev, next));
		return res;
	}

	constexpr bool stateTransitionRequiresSynchronization2(ResourceState2 prev, ResourceState2 next, bool is_image)
	{
		const bool res =
			!(accessIsRead2(prev._access) && accessIsRead2(next._access)) // Assuming that !read = write
			|| (is_image && layoutTransitionRequired(prev, next));
		return res;
	}


	class ResourceStateTracker
	{
	public:

		using BufferKey = typename BufferInstance::ResourceKey;
		using ImageKey = typename ImageViewInstance::ResourceKey;

		template<class K, class V, class Hasher = std::hash<K>, class Eq = std::equal_to<K>>
		using Map = std::unordered_map<K, V, Hasher, Eq>;

		class BufferStates
		{
		protected:

			using Range = typename BufferInstance::Range;

			BufferInstance* _buffer = nullptr;

			Map<Range, ResourceState2> _states;

		public:

			BufferStates(BufferInstance* buffer = nullptr);

			ResourceState2 getState(Range const& r);

			void setState(Range const& r, ResourceState2 const& state);

		};

		class ImageStates
		{
		protected:

			using Range = VkImageSubresourceRange;

			struct Hasher
			{
				size_t operator()(Range const& r) const
				{
					const std::hash<size_t> hs;
					const std::hash<uint32_t> hu;
					// don't consider the aspect in the hash since it should be the same
					return hu(r.baseMipLevel) ^ hu(r.levelCount) ^ hu(r.baseArrayLayer) ^ hu(r.layerCount);
				}
			};

			ImageInstance* _image = nullptr;

			Map<Range, ResourceState2, Hasher> _states;

		public:

			ImageStates(ImageInstance* image = nullptr);

			ResourceState2 getState(Range const& r);

			void setState(Range const& r, ResourceState2 const& state);

		};

	protected:


		Map<size_t, ImageStates> _images;
		Map<size_t, BufferStates> _buffers;


	public:

		ResourceStateTracker();

		void registerImage(ImageInstance* image);

		void registerBuffer(BufferInstance* buffer);

		void releaseImage(ImageInstance* image);

		void releaseBuffer(BufferInstance* buffer);

		ResourceState2 getImageState(ImageKey const& key);

		ResourceState2 getImageState(ImageViewInstance* view);

		ResourceState2 getBufferState(BufferKey const& key);

		void setImageState(ImageKey const& key, ResourceState2 const& state);

		void setImageState(ImageViewInstance* view, ResourceState2 const& state);

		void setBufferState(BufferKey const& key, ResourceState2 const& state);

	};
}