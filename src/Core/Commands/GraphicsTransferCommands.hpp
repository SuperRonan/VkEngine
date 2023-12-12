#pragma once

#include "DeviceCommand.hpp"
#include <Core/VkObjects/ImageView.hpp>
#include <Core/Execution/CompletionCallback.hpp>
#include <Core/Execution/ResourcesToUpload.hpp>

namespace vkl
{
	class GraphicsTransferCommand : public DeviceCommand
	{
	public:
		GraphicsTransferCommand(VkApplication* app, std::string const& name) :
			DeviceCommand(app, name)
		{}

		virtual ~GraphicsTransferCommand() override
		{}

		virtual void init() override
		{}
	};

	class BlitImage : public GraphicsTransferCommand
	{
	protected:

		std::shared_ptr<ImageView> _src, _dst;
		std::vector<VkImageBlit> _regions;
		VkFilter _filter = VK_FILTER_NEAREST;

		struct BlitInfoInstance
		{
			std::shared_ptr<ImageViewInstance> src = nullptr;
			std::shared_ptr<ImageViewInstance> dst = nullptr;
			std::vector<VkImageBlit> regions = {};
			VkFilter filter = VK_FILTER_MAX_ENUM;
		};

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

		void execute(ExecutionContext& context, BlitInfoInstance const& bi);

		ExecutionNode getExecutionNode(RecordContext& ctx, BlitInfo const& bi);

		virtual ExecutionNode getExecutionNode(RecordContext& ctx) override;

		Executable with(BlitInfo const& bi);

		Executable operator()(BlitInfo const& bi)
		{
			return with(bi);
		}

		BlitInfo getDefaultBlitInfo()
		{
			return BlitInfo{
				.src = _src,
				.dst = _dst,
				.regions = _regions,
				.filter = _filter,
			};
		}
	};

	class ComputeMips : public GraphicsTransferCommand
	{
	protected:

		std::vector<std::shared_ptr<ImageView>> _targets;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<std::shared_ptr<ImageView>> targets;
		};
		using CI = CreateInfo;

		ComputeMips(CreateInfo const& ci) :
			GraphicsTransferCommand(ci.app, ci.name),
			_targets(ci.targets)
		{

		}

		virtual ~ComputeMips() override {};

		using MipsInfo = AsynchMipsCompute;

		struct ExecInfo
		{
			std::vector<MipsInfo> targets;
		};
		using EI = ExecInfo;

		void execute(ExecutionContext& ctx, ExecInfo const& ei);

		ExecutionNode getExecutionNode(RecordContext& ctx, ExecInfo const& ei);

		virtual ExecutionNode getExecutionNode(RecordContext& ctx) override;

		Executable with(ExecInfo const& ei);

		Executable operator()(ExecInfo const& ei)
		{
			return with(ei);
		}

		ExecInfo getDefaultExecInfo()
		{
			ExecInfo res{
			};
			res.targets.resize(_targets.size());
			for (size_t i = 0; i < res.targets.size(); ++i)
			{
				res.targets[i] = {
					.target = _targets[i]->instance(),
				};
			}
			return res;
		}
	};



	class ResolveImage : public GraphicsTransferCommand
	{
		// TODO
	};

	



	// Actually a Compute or Graphics Command (doesn't work on Transfer Queues) for color images
	// Only on Graphics for depth stencil
	class ClearImage : public GraphicsTransferCommand
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

		struct ClearInfoInstance
		{
			std::shared_ptr<ImageViewInstance> view;
			VkClearValue value;
		};

		ClearImage(CreateInfo const& ci);

		void execute(ExecutionContext& context, ClearInfoInstance const& ci);

		ExecutionNode getExecutionNode(RecordContext& ctx, ClearInfo const& ci);

		virtual ExecutionNode getExecutionNode(RecordContext& ctx) override;

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
}