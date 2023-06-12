#pragma once

#include "DeviceCommand.hpp"
#include <Core/DynamicValue.hpp>

namespace vkl
{
	class TransferCommand : public DeviceCommand
	{
	protected:

	public:
		TransferCommand(VkApplication * app, std::string const& name):
			DeviceCommand(app, name)
		{}

		virtual ~TransferCommand() override
		{}
	};

	class BlitImage : public TransferCommand
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

		Executable with(BlitInfo const& bi);

		Executable operator()(BlitInfo const& bi)
		{
			return with(bi);
		}

	};

	class CopyImage : public TransferCommand
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

		Executable with(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return with(cinfo);
		}
	};

	class CopyBufferToImage : public TransferCommand
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

		Executable with(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return with(cinfo);
		}
	};

	class CopyBuffer : public TransferCommand
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

		Executable with(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return with(cinfo);
		}
	};

	class FillBuffer : public TransferCommand
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

		Executable with(FillInfo const& fi);

		Executable operator()(FillInfo const& fi)
		{
			return with(fi);
		}
	};

	class ClearImage : public TransferCommand
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

		virtual void execute(ExecutionContext& context) override;

		Executable with(ClearInfo const& ci);

		Executable operator()(ClearInfo const& ci)
		{
			return with(ci);
		}
	};

	class UpdateBuffer : public TransferCommand
	{
	protected:

		ObjectView _src;
		std::shared_ptr<Buffer> _dst;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			ObjectView src;
			std::shared_ptr<Buffer> dst = nullptr;
		};
		using CI = CreateInfo;

		UpdateBuffer(CreateInfo const& ci);

		virtual ~UpdateBuffer() override;

		struct UpdateInfo
		{
			ObjectView src;
			std::shared_ptr<Buffer> dst = nullptr;
			VkDeviceSize offset = 0;
		};
		using UI = UpdateInfo;

		void execute(ExecutionContext& ctx, UpdateInfo const& ui);

		virtual void execute(ExecutionContext& ctx) override;

		Executable with(UpdateInfo const& ui);

		Executable operator()(UpdateInfo const& ui)
		{
			return with(ui);
		}
	};

	class ComputeMips : public TransferCommand
	{
	protected:

		std::shared_ptr<ImageView> _target;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> target;
		};

		ComputeMips(CreateInfo const& ci):
			TransferCommand(ci.app, ci.name),
			_target(ci.target)
		{

		}

		virtual ~ComputeMips() override {};

		struct ExecInfo
		{
			std::shared_ptr<ImageView> target;
		};
		using EI = ExecInfo;

		void execute(ExecutionContext& ctx, ExecInfo const& ei);

		virtual void execute(ExecutionContext& ctx) override;

		Executable with(ExecInfo const& ei);

		Executable operator()(ExecInfo const& ei)
		{
			return with(ei);
		}
	};

}