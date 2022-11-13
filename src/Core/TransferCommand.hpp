#pragma once

#include "DeviceCommand.hpp"

namespace vkl
{
	class BlitImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<ImageView> _src, _dst;
		std::vector<VkImageBlit> _regions;
		VkFilter _filter = VK_FILTER_NEAREST;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkImageBlit> regions = {};
			VkFilter filter = VK_FILTER_NEAREST;
		};
		using CI = CreateInfo;

		BlitImage(CreateInfo const& ci);

		void setImages(std::shared_ptr<ImageView> src, std::shared_ptr<ImageView> dst);

		void setRegions(std::vector<VkImageBlit> const& regions = {});

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;

	};

	class CopyImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<ImageView> _src, _dst;
		std::vector<VkImageCopy> _regions;

	public:

		struct CreateInfo 
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkImageCopy> regions = {};
		};
		using CI = CreateInfo;

		CopyImage(CreateInfo const& ci);

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;
	};

	class CopyBufferToImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<Buffer> _src = nullptr;
		std::shared_ptr<ImageView> _dst = nullptr;
		std::vector<VkBufferImageCopy> _regions = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Buffer> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkBufferImageCopy> regions = {};
		};
		using CI = CreateInfo;

		CopyBufferToImage(CreateInfo const& ci);

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;
		
	};

	class CopyBuffer : public DeviceCommand
	{
	protected:

		std::shared_ptr<Buffer> _src;
		std::shared_ptr<Buffer> _dst;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Buffer> src = nullptr;
			std::shared_ptr<Buffer> dst = nullptr;
			// TODO copy range
		};

		using CI = CreateInfo;

		CopyBuffer(CreateInfo const& ci);

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;
	};

	class FillBuffer : public DeviceCommand
	{
	protected:

		std::shared_ptr<Buffer> _buffer = nullptr;

		VkDeviceSize _begin = 0;
		VkDeviceSize _size = VK_WHOLE_SIZE;
		uint32_t _value = 0u;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Buffer> buffer = nullptr;
			VkDeviceSize begin = 0;
			VkDeviceSize size = 0;
			uint32_t value;
		};
		using CI = CreateInfo;

		FillBuffer(CreateInfo const& ci);

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;
	};

	class ClearImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<ImageView> _view;
		VkClearValue _value;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> view = nullptr;
			VkClearValue value = VkClearValue{ .color = VkClearColorValue{.int32{0, 0, 0, 0}} };
		};
		using CI = CreateInfo;

		ClearImage(CreateInfo const& ci);

		void init() override;

		void execute(ExecutionContext& context) override;
	};
}