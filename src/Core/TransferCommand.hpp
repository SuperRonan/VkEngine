#pragma once

#include "DeviceCommand.hpp"
#include "DynamicValue.hpp"

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

		struct BlitInfo
		{
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkImageBlit> regions = {};
			VkFilter filter = VK_FILTER_MAX_ENUM;
		};
		using BI = BlitInfo;

		BlitImage(CreateInfo const& ci);

		virtual void init() override {};

		void execute(ExecutionContext& context, BlitInfo const& bi);

		virtual void execute(ExecutionContext& context) override;

		Executable executeWith(BlitInfo const& bi);

		Executable operator()(BlitInfo const& bi)
		{
			return executeWith(bi);
		}

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

		struct CopyInfo
		{
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkImageCopy> regions = {};
		};

		CopyImage(CreateInfo const& ci);

		virtual void init() override;

		void execute(ExecutionContext& ctx, CopyInfo const& cinfo);

		virtual void execute(ExecutionContext& context) override;

		Executable executeWith(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return executeWith(cinfo);
		}
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

		struct CopyInfo
		{
			std::shared_ptr<Buffer> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkBufferImageCopy> regions = {};
		};

		CopyBufferToImage(CreateInfo const& ci);

		virtual void init() override;

		void execute(ExecutionContext& context, CopyInfo const& cinfo);

		virtual void execute(ExecutionContext& context) override;

		Executable executeWith(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return executeWith(cinfo);
		}
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

		struct CopyInfo
		{
			std::shared_ptr<Buffer> src = nullptr;
			std::shared_ptr<Buffer> dst = nullptr;
		};

		CopyBuffer(CreateInfo const& ci);

		virtual void init() override;

		void execute(ExecutionContext& ctx, CopyInfo const& cinfo);

		virtual void execute(ExecutionContext& context) override;

		Executable executeWith(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return executeWith(cinfo);
		}
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
			VkDeviceSize size = VK_WHOLE_SIZE;
			uint32_t value = 0;
		};
		using CI = CreateInfo;

		struct FillInfo
		{
			std::shared_ptr<Buffer> buffer = nullptr;
			std::optional<VkDeviceSize> begin = {};
			std::optional<VkDeviceSize> size = {};
			std::optional<uint32_t> value = {};
		};

		FillBuffer(CreateInfo const& ci);

		virtual void init() override;

		void execute(ExecutionContext& context, FillInfo const& fi);

		virtual void execute(ExecutionContext& context) override;

		Executable executeWith(FillInfo const& fi);

		Executable operator()(FillInfo const& fi)
		{
			return executeWith(fi);
		}
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

		struct ClearInfo
		{
			std::shared_ptr<ImageView> view = nullptr;
			std::optional<VkClearValue> value = {};
		};

		ClearImage(CreateInfo const& ci);

		void init() override;

		void execute(ExecutionContext& context, ClearInfo const& ci);

		void execute(ExecutionContext& context) override;

		Executable executeWith(ClearInfo const& ci);

		Executable operator()(ClearInfo const& ci)
		{
			return executeWith(ci);
		}
	};
}