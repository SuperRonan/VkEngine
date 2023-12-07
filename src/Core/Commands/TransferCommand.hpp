#pragma once

#include "DeviceCommand.hpp"
#include <Core/DynamicValue.hpp>
#include <Core/Execution/ResourcesHolder.hpp>

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

		virtual void init() override
		{}
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

		void execute(ExecutionContext& ctx, CopyInfo const& cinfo);

		ExecutionNode getExecutionNode(RecordContext & ctx, CopyInfo const& ci);

		virtual ExecutionNode getExecutionNode(RecordContext & ctx) override;

		Executable with(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return with(cinfo);
		}

		CopyInfo getDefaultCopyInfo()
		{
			return CopyInfo{
				.src = _src,
				.dst = _dst,
				.regions = _regions,
			};
		}
	};

	class CopyBufferToImage : public TransferCommand
	{
	protected:

		std::shared_ptr<Buffer> _src = nullptr;
		DynamicValue<Range_st> _range = {};
		std::shared_ptr<ImageView> _dst = nullptr;
		std::vector<VkBufferImageCopy> _regions = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Buffer> src = nullptr;
			DynamicValue<Range_st> range = Range_st{0, 0};
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkBufferImageCopy> regions = {};
		};
		using CI = CreateInfo;

		struct CopyInfo
		{
			std::shared_ptr<Buffer> src = nullptr;
			Range_st range;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkBufferImageCopy> regions = {};
			uint32_t default_buffer_row_length = 0;
			uint32_t default_buffer_image_height = 0;
		};

		CopyBufferToImage(CreateInfo const& ci);

		void execute(ExecutionContext& context, CopyInfo const& cinfo);

		ExecutionNode getExecutionNode(RecordContext& ctx, CopyInfo const& ci);

		virtual ExecutionNode getExecutionNode(RecordContext& ctx) override;

		Executable with(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return with(cinfo);
		}

		CopyInfo getDefaultCopyInfo()
		{
			return CopyInfo {
				.src = _src,
				.range = _range.value(),
				.dst = _dst,
				.regions = _regions,
			};
		}
	};

	class CopyBuffer : public TransferCommand
	{
	protected:

		std::shared_ptr<Buffer> _src = nullptr;
		DynamicValue<size_t> _src_offset = {};
		std::shared_ptr<Buffer> _dst = nullptr;
		DynamicValue<size_t> _dst_offset = {};
		DynamicValue<size_t> _size = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Buffer> src = nullptr;
			DynamicValue<size_t> src_offset = 0;
			std::shared_ptr<Buffer> dst = nullptr;
			DynamicValue<size_t> dst_offset = 0;
			DynamicValue<size_t> size = 0; // 0 means VK_WHOLE_SIZE
			// TODO copy range
		};
		using CI = CreateInfo;

		struct CopyInfo
		{
			std::shared_ptr<Buffer> src = nullptr;
			size_t src_offset = 0;
			std::shared_ptr<Buffer> dst = nullptr;
			size_t dst_offset = 0;
			size_t size = 0;
		};

		CopyBuffer(CreateInfo const& ci);

		void execute(ExecutionContext& ctx, CopyInfo const& cinfo);

		ExecutionNode getExecutionNode(RecordContext & ctx, CopyInfo const& cinfo);

		virtual ExecutionNode getExecutionNode(RecordContext & ctx) override;

		Executable with(CopyInfo const& cinfo);

		Executable operator()(CopyInfo const& cinfo)
		{
			return with(cinfo);
		}

		CopyInfo getDefaultCopyInfo()
		{
			return CopyInfo{
				.src = _src,
				.src_offset = _src_offset.value(),
				.dst = _dst,
				.dst_offset = _dst_offset.value(),
				.size = _size.value(),
			};
		}
	};

	class FillBuffer : public TransferCommand
	{
	protected:

		std::shared_ptr<Buffer> _buffer = nullptr;
		DynamicValue<Range_st> _range = {};
		uint32_t _value = 0u;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Buffer> buffer = nullptr;
			DynamicValue<Range_st> range = Range_st{.begin = 0, .len = 0};
			uint32_t value = 0;
		};
		using CI = CreateInfo;

		struct FillInfo
		{
			std::shared_ptr<Buffer> buffer = nullptr;
			std::optional<Range_st> range = {};
			std::optional<uint32_t> value = {};
		};

		FillBuffer(CreateInfo const& ci);

		void execute(ExecutionContext& context, FillInfo const& fi);

		ExecutionNode getExecutionNode(RecordContext & ctx, FillInfo const& fi);

		virtual ExecutionNode getExecutionNode(RecordContext & ctx) override;

		Executable with(FillInfo const& fi);

		Executable operator()(FillInfo const& fi)
		{
			return with(fi);
		}

		FillInfo getDefaultFillInfo()
		{
			return FillInfo{
				.buffer = _buffer,
				.range = _range.value(),
				.value = _value,
			};
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

		void execute(ExecutionContext& context, ClearInfo const& ci);

		ExecutionNode getExecutionNode(RecordContext & ctx, ClearInfo const& ci);

		virtual ExecutionNode getExecutionNode(RecordContext & ctx) override;

		Executable with(ClearInfo const& ci);

		Executable operator()(ClearInfo const& ci)
		{
			return with(ci);
		}

		ClearInfo getDefaultClearInfo()
		{
			return ClearInfo{
				.view = _view,
				.value = _value,
			};
		}
	};

	class UpdateBuffer : public TransferCommand
	{
	protected:

		ObjectView _src;
		std::shared_ptr<Buffer> _dst;
		DynamicValue<size_t> _offset;
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			ObjectView src;
			std::shared_ptr<Buffer> dst = nullptr;
			DynamicValue<size_t> offset = 0;
		};
		using CI = CreateInfo;

		UpdateBuffer(CreateInfo const& ci);

		virtual ~UpdateBuffer() override;

		struct UpdateInfo
		{
			ObjectView src;
			std::shared_ptr<Buffer> dst = nullptr;
			std::optional<size_t> offset = {};
		};
		using UI = UpdateInfo;

		void execute(ExecutionContext& ctx, UpdateInfo const& ui);

		ExecutionNode getExecutionNode(RecordContext & ctx, UpdateInfo const& ui);

		virtual ExecutionNode getExecutionNode(RecordContext & ctx) override;

		Executable with(UpdateInfo const& ui);

		Executable operator()(UpdateInfo const& ui)
		{
			return with(ui);
		}

		UpdateInfo getDefaultUpdateInfo()
		{
			return UpdateInfo{
				.src = _src,
				.dst = _dst,
				.offset = _offset.value(),
			};
		}
	};



	class UploadBuffer : public TransferCommand
	{
	protected:

		std::shared_ptr<Buffer> _dst = nullptr;
		DynamicValue<size_t> _offset = {};
		ObjectView _src;

		bool _use_update_buffer_ifp = false;

	public:

		struct CreateInfo 
		{
			VkApplication* app = nullptr;
			std::string name = {};
			ObjectView src = {};
			std::shared_ptr<Buffer> dst = nullptr;
			DynamicValue<size_t> offset = 0;
			bool use_update_buffer_ifp = false;
		};
		using CI = CreateInfo;

		UploadBuffer(CreateInfo const& ci);

		virtual ~UploadBuffer() override = default;

		struct UploadInfo
		{
			std::vector<PositionedObjectView> sources = {};
			std::shared_ptr<Buffer> dst = nullptr;
			std::optional<bool> use_update_buffer_ifp = {};
		};
		using UI = UploadInfo;

		void execute(ExecutionContext& ctx, UploadInfo const& ui, bool use_update, Buffer::Range buffer_range);

		ExecutionNode getExecutionNode(RecordContext & ctx, UploadInfo const& ui);

		ExecutionNode getExecutionNode(RecordContext & ctx);

		Executable with(UploadInfo const& ui);

		Executable operator()(UploadInfo const& ui)
		{
			return with(ui);
		}

		UploadInfo getDefaultUploadInfo()
		{
			return UploadInfo{
				.sources = {PositionedObjectView{.obj = _src, .pos = _offset.valueOr(0), }},
				.dst = _dst,
				.use_update_buffer_ifp = _use_update_buffer_ifp,
			};
		}
	};

	class UploadImage : public TransferCommand
	{
	protected:

		ObjectView _src;
		std::shared_ptr<ImageView> _dst = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			ObjectView src = {};
			std::shared_ptr<ImageView> dst = nullptr;
		};
		using CI = CreateInfo;

		UploadImage(CreateInfo const& ci);

		virtual ~UploadImage() override = default;

		struct UploadInfo
		{
			ObjectView src = {};
			uint32_t buffer_row_length = 0;
			uint32_t buffer_image_height = 0;
			std::shared_ptr<ImageView> dst = nullptr;
		};
		using UI = UploadInfo;

		void execute(ExecutionContext& ctx, UploadInfo const& ui);

		ExecutionNode getExecutionNode(RecordContext & ctx, UploadInfo const& ui);

		virtual ExecutionNode getExecutionNode(RecordContext & ctx) override;

		Executable with(UploadInfo const& ui);

		Executable operator()(UploadInfo const& ui)
		{
			return with(ui);
		}

		UploadInfo getDefaultUploadInfo()
		{
			return UploadInfo{
				.src = _src,
				.dst = _dst,
			};
		}
	};


	class UploadResources : public TransferCommand
	{
	protected:
	
		std::shared_ptr<ResourcesHolder> _holder;

		struct BufferUploadExtraInfo
		{
			bool use_update = false;
			Buffer::Range range = {};
		};

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<ResourcesHolder> holder = nullptr;
		};
		using CI = CreateInfo;

		UploadResources(CreateInfo const& ci);

		virtual ~UploadResources() override = default;

		struct UploadInfo
		{
			ResourcesToUpload upload_list = {};
		};
		using UI = UploadInfo;

		void execute(ExecutionContext & ctx, UploadInfo const& ui, std::vector<BufferUploadExtraInfo> const& extra_buffer_info);

		ExecutionNode getExecutionNode(RecordContext & ctx, UploadInfo const& ui);

		virtual ExecutionNode getExecutionNode(RecordContext & ctx) override;

		Executable with(UploadInfo const& ui);

		Executable operator()(UploadInfo const& ui)
		{
			return with(ui);
		}
	};
}