#pragma once

#include <vkl/App/VkApplication.hpp>
#include "Image.hpp"

namespace vkl
{
	namespace GUI
	{
		class ImageViewInstanceInspector;
		class ImageViewInspector;
	}
	class ImageViewInstance : public InstanceBase<VkImageView>
	{
	public:

		using Parent = InstanceBase<VkImageView>;

		static constexpr const char* ClassName = "Image View";

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			size_t tick = {};
			std::shared_ptr<ImageInstance> image = nullptr;
			VkImageViewCreateInfo ci = {};
		};
		using CI = CreateInfo;

	protected:

		static std::atomic<size_t> _instance_counter;
		
		std::shared_ptr<ImageInstance> _image = nullptr;

		VkImageViewCreateInfo _ci = {};

		size_t _unique_id = 0;

		void create();

		void destroy();

	public:

		ImageViewInstance() = delete;


		ImageViewInstance(ImageViewInstance const&) = delete;
		ImageViewInstance(ImageViewInstance &&) = delete;

		ImageViewInstance& operator=(ImageViewInstance const&) = delete;
		ImageViewInstance& operator=(ImageViewInstance &&) = delete;

		ImageViewInstance(CreateInfo const& ci);

		ImageViewInstance(std::shared_ptr<ImageInstance> const& image);

		virtual ~ImageViewInstance();

		std::shared_ptr<ImageInstance> const& image()const
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

		VkImageSubresourceRange finiteRange() const
		{
			return _image->finiteRange(_ci.subresourceRange);
		}

		Range32u finiteArrayRange() const
		{
			return Range32u{.begin = _ci.subresourceRange.baseArrayLayer, .len =  _image->finiteRange(_ci.subresourceRange).layerCount};
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

		using InspectorType = GUI::ImageViewInstanceInspector;
		friend class InspectorType;

		virtual std::shared_ptr<GUI::Panel> makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx) override;
	};

	class ImageView : public InstanceHolder<ImageViewInstance>
	{
	public:

		using Parent = InstanceHolder<ImageViewInstance>;

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = "";
			std::shared_ptr<Image> image = nullptr;
			VkImageViewType type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			Dyn<VkFormat> format;
			VkComponentMapping components = defaultComponentMapping();
			Dyn<VkImageSubresourceRange> range = {};
			bool create_on_construct = false;
			Dyn<bool> hold_instance = true;
		};

		using CI = CreateInfo;


	protected:

		std::shared_ptr<Image> _image = nullptr;
		VkImageViewType _type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		Dyn<VkFormat> _format;
		VkComponentMapping _components = defaultComponentMapping();
		Dyn<VkImageSubresourceRange> _range = {};

		void setImageInvalidationCallback();

		void constructorBody(bool create_instance);

		virtual void updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res) override;

	public:

		ImageView(CreateInfo const& ci);

		ImageView(Image::CreateInfo const& ci);

		ImageView(std::shared_ptr<ImageViewInstance> const& inst);

		ImageView(std::shared_ptr<ImageInstance> const& image_inst);

		virtual ~ImageView() override;

		void createInstance(size_t tick = 0);

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

		Dyn<VkSampleCountFlagBits> const& sampleCount() const
		{
			return image()->sampleCount();
		}

		constexpr VkComponentMapping components()const
		{
			return _components;
		}

		constexpr const Dyn<VkImageSubresourceRange>& range()const
		{
			return _range;
		}

		VkImageSubresourceRange finiteRange() const
		{
			if(_range)
			{
				return _image->finiteRange(_range.value());
			}
			else
			{
				VkImageAspectFlags aspect = getImageAspectFromFormat(_format.value());
				return VkImageSubresourceRange{
					.aspectMask = aspect,
					.baseMipLevel = 0,
					.levelCount = _image->mips().value(),
					.baseArrayLayer = 0,
					.layerCount = _image->layers().value(),
				};
			}
		}

		Range32u finiteArrayRange() const
		{
			if (_range)
			{
				VkImageSubresourceRange range = _range.value();
				range.baseMipLevel = 0;
				range.levelCount = 0;
				return Range32u{.begin = range.baseArrayLayer, .len = _image->finiteRange(range).layerCount};
			}
			else
			{
				return Range32u{.begin = 0, .len = _image->layers().value()};
			}
		}

		using InspectorType = GUI::ImageViewInspector;
		friend class InspectorType;

		virtual std::shared_ptr<GUI::Panel> makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx) override;
	};

	VKL_DEFINE_DESCRIPTOR_INSTANCE_POINTERS(ImageView)
}