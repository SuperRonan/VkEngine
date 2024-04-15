#pragma once

#include "ShaderCommand.hpp"

namespace vkl
{
	class ComputeCommandNode : public ShaderCommandNode
	{
	public:
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		ComputeCommandNode(CreateInfo const& ci);

		struct DispatchCallInfo
		{
			std::string name = {};
			// In WorkGroups
			VkExtent3D extent = makeZeroExtent3D();
			PushConstant pc = {};
			std::shared_ptr<DescriptorSetAndPoolInstance> set = nullptr;
		};

		MyVector<DispatchCallInfo> _dispatch_list = {};

		virtual void clear() override;

		virtual void execute(ExecutionContext& ctx) override;
	};

	class ComputeCommand : public ShaderCommand
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path shader_path;
			DynamicValue<VkExtent3D> extent = {};
			bool dispatch_threads = false;
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::vector<ShaderBindingDescription> bindings = {};
			DynamicValue<std::vector<std::string>> definitions = std::vector<std::string>();
		};
		using CI = CreateInfo;

		
		struct DispatchCallInfo
		{
			std::string name = {};
			VkExtent3D extent = makeZeroExtent3D();
			PushConstant pc = {};
			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};

		struct DispatchInfo
		{
			bool dispatch_threads = false;
			Array<DispatchCallInfo> dispatch_list;
		};

	protected:

		std::filesystem::path _shader_path;
		DynamicValue<std::vector<std::string>> _definitions;

		std::shared_ptr<ComputeProgram> _program = nullptr;
		DynamicValue<VkExtent3D> _extent = {};
		bool _dispatch_threads = false;

	public:

		ComputeCommand(CreateInfo const& ci);

		virtual ~ComputeCommand() override = default;


		virtual void init() override;

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, DispatchInfo const& di);

		Executable with(DispatchInfo const& di);

		Executable operator()(DispatchInfo const& di)
		{
			return with(di);
		}

		struct SingleDispatchInfo {
			std::optional<VkExtent3D> extent = {};
			std::optional<bool> dispatch_threads = {};
			PushConstant pc = {};
			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};

		Executable with(SingleDispatchInfo const& sdi)
		{
			VkExtent3D extent;
			if (sdi.extent.has_value())
			{
				extent = sdi.extent.value();
			}
			else
			{
				assert(_extent.hasValue());
				extent = _extent.value();
			}
			DispatchInfo di{
				.dispatch_threads = sdi.dispatch_threads.value_or(_dispatch_threads),
				.dispatch_list = {
					DispatchCallInfo{
						.extent = extent,
						.pc = sdi.pc,
						.set = sdi.set,
					},
				},
			};
			return with(di);
		}

		Executable operator()(SingleDispatchInfo const& sdi)
		{
			return with(sdi);
		}

		virtual bool updateResources(UpdateContext & ctx)override;

		void setDispatchSize(DynamicValue<VkExtent3D> size)
		{
			_extent = size;
		}

		constexpr void setDispatchType(bool type_is_threads)
		{
			_dispatch_threads = type_is_threads;
		}
		
		constexpr const DynamicValue<VkExtent3D> & getDispatchSize()const
		{
			return _extent;
		}

		VkExtent3D getWorkgroupsDispatchSize(VkExtent3D threads)const
		{
			const std::shared_ptr<ComputeProgramInstance> & prog = std::reinterpret_pointer_cast<ComputeProgramInstance>(_program->instance());
			const VkExtent3D lcl = prog->localSize();
			return VkExtent3D{
				.width = std::divCeil(threads.width, lcl.width),
				.height = std::divCeil(threads.height, lcl.height),
				.depth = std::divCeil(threads.depth, lcl.depth),
			};
		}
	};
}