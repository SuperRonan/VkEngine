#pragma once

#include <Core/App/VkApplication.hpp>
#include "Image.hpp"
#include <array>
#include <optional>

namespace vkl
{
	class ImageViewInstance : public AbstractInstance
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageInstance> image = nullptr;
			VkImageViewCreateInfo ci = {};
		};
		using CI = CreateInfo;

	protected:

		static std::atomic<size_t> _instance_counter;
		
		std::shared_ptr<ImageInstance> _image = nullptr;

		VkImageViewCreateInfo _ci = {};

		VkImageView _view = VK_NULL_HANDLE;

		size_t _unique_id = 0;

		void create();

		void destroy();

		void setVkNameIFP();

	public:

		ImageViewInstance() = delete;


		ImageViewInstance(ImageViewInstance const&) = delete;
		ImageViewInstance(ImageViewInstance &&) = delete;

		ImageViewInstance& operator=(ImageViewInstance const&) = delete;
		ImageViewInstance& operator=(ImageViewInstance &&) = delete;

		ImageViewInstance(CreateInfo const& ci);

		virtual ~ImageViewInstance();

		constexpr VkImageView view()const
		{
			return _view;
		}

		constexpr auto handle()const
		{
			return view();
		}

		constexpr operator VkImageView()const
		{
			return view();
		}

		auto image()const
		{
			return _image;
		}

		constexpr const VkImageViewCreateInfo& createInfo()const
		{
			return _ci;
		}

		constexpr size_t uniqueId() const
		{
			return _unique_id;
		}

		struct ResourceKey
		{
			size_t id = 0;
			VkImageSubresourceRange range = {};
		};

		constexpr ResourceKey getResourceKey() const
		{
			return ResourceKey{
				.id = _image->uniqueId(),
				.range = _ci.subresourceRange,
			};
		}
		
		decltype(auto) getState(size_t tid)const
		{
			return _image->getState(tid, _ci.subresourceRange);
		}

		void setState(size_t tid, ResourceState2 const& state)
		{
			_image->setState(tid, _ci.subresourceRange, state);
		}
	};

	class ImageView : public InstanceHolder<ImageViewInstance>
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = "";
			std::shared_ptr<Image> image = nullptr;
			std::optional<Image::CreateInfo> image_ci = {};
			VkImageViewType type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			Dyn<VkFormat> format;
			VkComponentMapping components = defaultComponentMapping();
			std::optional<VkImageSubresourceRange> range = {};
			bool create_on_construct = false;
		};

		using CI = CreateInfo;


	protected:

		std::shared_ptr<Image> _image = nullptr;
		VkImageViewType _type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		Dyn<VkFormat> _format;
		VkComponentMapping _components = defaultComponentMapping();
		VkImageSubresourceRange _range = {};
		
	public:

		ImageView(CreateInfo const& ci);

		virtual ~ImageView() override;

		void createInstance();

		void destroyInstance();

		constexpr const auto& image()const
		{
			return _image;
		}

		constexpr VkImageViewType type()const
		{
			return _type;
		}

		Dyn<VkFormat> format()const
		{
			return _format;
		}

		constexpr VkComponentMapping components()const
		{
			return _components;
		}

		constexpr VkImageSubresourceRange range()const
		{
			return _range;
		}

		bool updateResource(UpdateContext & ctx);

		// StagingPool::StagingBuffer* copyToStaging2D(StagingPool& pool, void* data, uint32_t elem_size);
		
		// void recordSendStagingToDevice2D(VkCommandBuffer command_buffer, StagingPool::StagingBuffer* sb, VkImageLayout layout);
		
		//void recordTransitionLayout(VkCommandBuffer command, VkImageLayout src, VkAccessFlags src_access, VkPipelineStageFlags src_stage, VkImageLayout dst, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage);
		
	};
}