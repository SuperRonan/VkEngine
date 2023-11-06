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

		void execute(ExecutionContext& ctx, UploadInfo const& ui);

		virtual void execute(ExecutionContext& ctx) override;

		Executable with(UploadInfo const& ui);

		Executable operator()(UploadInfo const& ui)
		{
			return with(ui);
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

		virtual void execute(ExecutionContext& ctx) override;

		Executable with(UploadInfo const& ui);

		Executable operator()(UploadInfo const& ui)
		{
			return with(ui);
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

		void execute(ExecutionContext & ctx, UploadInfo const& ui);

		virtual void execute(ExecutionContext & ctx) override;

		Executable with(UploadInfo const& ui);

		Executable operator()(UploadInfo const& ui)
		{
			return with(ui);
		}
	};
}