#pragma once

#include "DeviceCommand.hpp"
#include <vkl/Core/DynamicValue.hpp>
#include <vkl/Execution/ResourcesHolder.hpp>

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
		MyVector<VkImageCopy2> _regions;


	public:

		struct CreateInfo 
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			MyVector<VkImageCopy2> regions = {};
		};
		using CI = CreateInfo;

		struct CopyInfo
		{
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			MyVector<VkImageCopy2> regions = {};
		};

		struct CopyInfoInstance
		{
			std::shared_ptr<ImageViewInstance> src = nullptr;
			std::shared_ptr<ImageViewInstance> dst = nullptr;
			MyVector<VkImageCopy2> regions = {};
		};

		CopyImage(CreateInfo const& ci);

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, CopyInfoInstance const& ci);
		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, CopyInfo const& ci);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		Executable with(CopyInfo const& cinfo);
		Executable with(CopyInfoInstance const& cinfo);

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

		BufferAndRange _src = {};
		std::shared_ptr<ImageView> _dst = nullptr;
		Array<VkBufferImageCopy2> _regions = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			BufferAndRange src = {};
			std::shared_ptr<ImageView> dst = nullptr;
			Array<VkBufferImageCopy2> regions = {};
		};
		using CI = CreateInfo;

		struct CopyInfo
		{
			BufferAndRange src = {};
			std::shared_ptr<ImageView> dst = nullptr;
			Array<VkBufferImageCopy2> regions = {};
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
				.dst = _dst,
				.regions = _regions,
			};
		}
	};

	class CopyImageToBuffer : public TransferCommand
	{
	protected:

		std::shared_ptr<ImageView> _src = nullptr;
		BufferAndRange _dst = {};
		Array<VkBufferImageCopy2> _regions = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			BufferAndRange dst = {};
			Array<VkBufferImageCopy2> regions = {};
		};
		using CI = CreateInfo;

		struct CopyInfo
		{
			std::shared_ptr<ImageView> src = nullptr;
			BufferAndRange dst = {};
			Array<VkBufferImageCopy2> regions = {};
			uint32_t default_buffer_row_length = 0;
			uint32_t default_buffer_image_height = 0;
		};

		CopyImageToBuffer(CreateInfo const& ci);

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx, CopyInfo const& ci);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

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

		std::shared_ptr<BufferPool> _staging_pool = nullptr;

	public:

		struct CreateInfo 
		{
			VkApplication* app = nullptr;
			std::string name = {};
			ObjectView src = {};
			std::shared_ptr<Buffer> dst = nullptr;
			DynamicValue<size_t> offset = 0;
			bool use_update_buffer_ifp = false;
			std::shared_ptr<BufferPool> staging_pool = nullptr;
		};
		using CI = CreateInfo;

		UploadBuffer(CreateInfo const& ci);

		virtual ~UploadBuffer() override = default;

		struct UploadInfo
		{
			Array<PositionedObjectView> sources = {};
			std::shared_ptr<Buffer> dst = nullptr;
			std::optional<bool> use_update_buffer_ifp = {};
			std::shared_ptr<BufferPool> staging_pool = nullptr;
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

		std::shared_ptr<BufferPool> _staging_pool = nullptr;
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			ObjectView src = {};
			std::shared_ptr<ImageView> dst = nullptr;
			std::shared_ptr<BufferPool> staging_pool = nullptr;
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
			std::shared_ptr<BufferPool> staging_pool = nullptr;
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
		std::shared_ptr<BufferPool> _staging_pool = nullptr;

		friend struct UploadResourcesTemplateProcessor;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<ResourcesHolder> holder = nullptr;
			std::shared_ptr<BufferPool> staging_pool = nullptr;
		};
		using CI = CreateInfo;

		UploadResources(CreateInfo const& ci);

		virtual ~UploadResources() override = default;

		struct UploadInfo
		{
			ResourcesToUpload upload_list = {};
			std::shared_ptr<BufferPool> staging_pool = nullptr;

			void clear()
			{
				upload_list.clear();
				staging_pool.reset();
			}
		};
		using UI = UploadInfo;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, UploadInfo const& ui);
		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, UploadInfo && ui);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		Executable with(UploadInfo const& ui);
		Executable with(UploadInfo && ui);

		Executable operator()(UploadInfo const& ui)
		{
			return with(ui);
		}
		Executable operator()(UploadInfo && ui)
		{
			return with(std::move(ui));
		}
	};


	using DownloadCallback = std::function<void(int, std::shared_ptr<PooledBuffer> const&)>;

	class DownloadBuffer : public TransferCommand
	{
	protected:

		BufferAndRange _src = {};
		void * _dst = nullptr;

		std::shared_ptr<BufferPool> _staging_pool = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			BufferAndRange src = {};
			void * dst = nullptr;
			std::shared_ptr<BufferPool> staging_pool = nullptr;
		};
		using CI = CreateInfo;

		DownloadBuffer(CreateInfo const& ci);

		virtual ~DownloadBuffer() override = default;

		struct DownloadInfo
		{
			BufferAndRange src = {};
			// If dst is nullptr, then no copy from staging buffer to dst
			// If dst != nullptr, then there is a copy from staging buffer to dst in completion callback
			void * dst = nullptr;
			std::shared_ptr<BufferPool> staging_pool = nullptr;
			DownloadCallback completion_callback = {};
		};
		using DI = DownloadInfo;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, DownloadInfo const& di);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		Executable with(DownloadInfo const& di);

		Executable operator()(DownloadInfo const& di)
		{
			return with(di);
		}
	};


	class DownloadImage : public TransferCommand
	{
	protected:

		std::shared_ptr<ImageView> _src = nullptr;
		void * _dst = nullptr;
		size_t _size = 0;

		std::shared_ptr<BufferPool> _staging_pool = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			void * dst = nullptr;
			size_t size = 0;
			std::shared_ptr<BufferPool> staging_pool = nullptr;
		};
		using CI = CreateInfo;

		DownloadImage(CreateInfo const& ci);

		virtual ~DownloadImage() override = default;

		struct DownloadInfo
		{
			std::shared_ptr<ImageView> src = nullptr;
			// If dst is nullptr, then no copy from staging buffer to dst
			// If dst != nullptr, then there is a copy from staging buffer to dst in completion callback
			void * dst = nullptr;
			size_t size = 0;
			uint32_t default_buffer_row_length = 0;
			uint32_t default_buffer_image_height = 0;
			std::shared_ptr<BufferPool> staging_pool = nullptr;
			DownloadCallback completion_callback = {};
		};
		using DI = DownloadInfo;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx, DownloadInfo const& di);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

		Executable with(DownloadInfo const& di);

		Executable operator()(DownloadInfo const& di)
		{
			return with(di);
		}
	};
}