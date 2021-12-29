#pragma once

#include "VkApplication.hpp"
#include "StagingPool.hpp"
#include <cassert>
#include <math.h>

namespace vkl
{

	class TextureBase : public VkObject
	{
	public:

		struct CreateInfo
		{
			VkImageType type;
			VkFormat format;
			VkExtent3D extent;
			bool use_mips = true;
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
			VkImageUsageFlags usage;
			std::vector<uint32_t> queues;

			VkImageViewType view_type;
			VkImageAspectFlags aspect;

			uint32_t elem_size;
		};

	protected:

		VkImageType _type;
		VkFormat _format;
		VkExtent3D _extent;
		uint32_t _mips;
		VkSampleCountFlagBits _samples;
		VkImageTiling _tiling;
		VkImageUsageFlags _usage;
		VkSharingMode _sharing_mode;
		std::vector<uint32_t> _queues;

		VkImageViewType _view_type;
		VkImageAspectFlags _aspect;

		uint32_t _elem_size;

		
		VmaAllocation _alloc;
		VkImage _image = VK_NULL_HANDLE;
		VkImageView _view = VK_NULL_HANDLE;

		

	public:

		constexpr TextureBase(VkApplication* app = nullptr):
			VkObject(app)
		{}

		TextureBase(VkApplication* app, CreateInfo const& ci, VkImageLayout layout=VK_IMAGE_LAYOUT_UNDEFINED);

		void create(CreateInfo const& ci, VkImageLayout layout= VK_IMAGE_LAYOUT_UNDEFINED);

		virtual ~TextureBase();
		
		void createImage(VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED);

		void createView();

		void destroyImage();

		void destroyView();

		void destroy();

		constexpr VkImage image()const
		{
			return _image;
		}

		constexpr VkImageView view()const
		{
			return _view;
		}

		constexpr VkExtent3D extent()const
		{
			return _extent;
		}

		StagingPool::StagingBuffer* copyToStaging2D(StagingPool& pool, void* data);
		
		void recordSendStagingToDevice2D(VkCommandBuffer command_buffer, StagingPool::StagingBuffer* sb, VkImageLayout layout);

		void recordTransitionLayout(VkCommandBuffer command, VkImageLayout prev, VkImageLayout next);
		
		void recordTransitionLayout(VkCommandBuffer command, VkImageLayout src, VkAccessFlags src_access, VkPipelineStageFlags src_stage, VkImageLayout dst, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage);

	};
}