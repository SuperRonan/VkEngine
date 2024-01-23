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

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, CopyInfo const& ci);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

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
		DynamicValue<Buffer::Range> _range = {};
		std::shared_ptr<ImageView> _dst = nullptr;
		Array<VkBufferImageCopy> _regions = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Buffer> src = nullptr;
			DynamicValue<Buffer::Range> range = {};
			std::shared_ptr<ImageView> dst = nullptr;
			Array<VkBufferImageCopy> regions = {};
		};
		using CI = CreateInfo;

		struct CopyInfo
		{
			std::shared_ptr<Buffer> src = nullptr;
			Buffer::Range range;
			std::shared_ptr<ImageView> dst = nullptr;
			Array<VkBufferImageCopy> regions = {};
			uint32_t default_buffer_row_length = 0;
			uint32_t default_buffer_image_height = 0;
		};

		CopyBufferToImage(CreateInfo const& ci);

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx, CopyInfo const& ci);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

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
			std::shared_ptr<Buffer> dst = nullptr;
			Array<VkBufferCopy2> regions = {};
		};

		struct CopyInfoInstance
		{
			std::shared_ptr<BufferInstance> src = nullptr;
			std::shared_ptr<BufferInstance> dst = nullptr;
			Array<VkBufferCopy2> regions = {};
		};

		CopyBuffer(CreateInfo const& ci);

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, CopyInfoInstance const& cii);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		Executable with(CopyInfo const& cinfo);

		Executable with(CopyInfoInstance const& cii);

		Executable operator()(CopyInfo const& cinfo)
		{
			return with(cinfo);
		}

		CopyInfoInstance getDefaultCopyInfo()
		{
			return CopyInfoInstance{
				.src = _src->instance(),
				.dst = _dst->instance(),
				.regions = {
					VkBufferCopy2{
						.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
						.pNext = nullptr,
						.srcOffset = _src_offset.valueOr(0),
						.dstOffset = _dst_offset.valueOr(0),
						.size = _size.valueOr(0),
					},
				},
			};
		}
	};

	class FillBuffer : public TransferCommand
	{
	protected:

		std::shared_ptr<Buffer> _buffer = nullptr;
		DynamicValue<Buffer::Range> _range = {};
		uint32_t _value = 0u;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Buffer> buffer = nullptr;
			DynamicValue<Buffer::Range> range = Buffer::Range{.begin = 0, .len = 0};
			uint32_t value = 0;
		};
		using CI = CreateInfo;

		struct FillInfo
		{
			std::shared_ptr<Buffer> buffer = nullptr;
			std::optional<Buffer::Range> range = {};
			std::optional<uint32_t> value = {};
		};

		FillBuffer(CreateInfo const& ci);

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, FillInfo const& fi);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

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

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, UpdateInfo const& ui);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

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
			Array<PositionedObjectView> sources = {};
			std::shared_ptr<Buffer> dst = nullptr;
			std::optional<bool> use_update_buffer_ifp = {};
		};
		using UI = UploadInfo;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, UploadInfo const& ui);

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx);

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

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, UploadInfo const& ui);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

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

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, UploadInfo const& ui);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		Executable with(UploadInfo const& ui);

		Executable operator()(UploadInfo const& ui)
		{
			return with(ui);
		}
	};
}