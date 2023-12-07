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

		void execute(ExecutionContext& context, BlitInfo const& bi);

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
					.target = _targets[i],
				};
			}
			return res;
		}
	};



	class ResolveImage : public GraphicsTransferCommand
	{
		// TODO
	};

	

}