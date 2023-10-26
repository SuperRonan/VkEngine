#pragma once

#include "ShaderCommand.hpp"

namespace vkl
{
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
			std::vector<DispatchCallInfo> dispatch_list;
		};
		using DI = DispatchInfo;

	protected:

		std::filesystem::path _shader_path;
		DynamicValue<std::vector<std::string>> _definitions;

		std::shared_ptr<ComputeProgram> _program = nullptr;
		DynamicValue<VkExtent3D> _extent = {};
		bool _dispatch_threads = false;

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context, DispatchInfo const& di) ;

	public:

		ComputeCommand(CreateInfo const& ci);

		virtual ~ComputeCommand() override = default;


		virtual void init() override;


		void execute(ExecutionContext& context, DispatchInfo const& di);
		
		virtual void execute(ExecutionContext& context) override;

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
			DispatchInfo di{
				.dispatch_threads = sdi.dispatch_threads.value_or(_dispatch_threads),
				.dispatch_list = {
					DispatchCallInfo{
						.extent = sdi.extent.value_or(_extent.value()),
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
			const std::shared_ptr<ComputeProgramInstance> & prog = std::dynamic_pointer_cast<ComputeProgramInstance>(_program->instance());
			return VkExtent3D{
				.width = std::divCeil(threads.width, prog->localSize().width),
				.height = std::divCeil(threads.height, prog->localSize().height),
				.depth = std::divCeil(threads.depth, prog->localSize().depth),
			};
		}
	};
}