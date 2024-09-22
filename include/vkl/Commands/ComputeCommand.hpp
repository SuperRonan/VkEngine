#pragma once

#include "ShaderCommand.hpp"
#include <vkl/VkObjects/ComputePipeline.hpp>

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
			Index name_begin = 0;
			Index pc_begin = 0;
			uint32_t name_size = 0;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			VkExtent3D extent = makeUniformExtent3D(0); // In WorkGroups
			std::shared_ptr<DescriptorSetAndPoolInstance> set = nullptr;
		};

		MyVector<DispatchCallInfo> _dispatch_list = {};

		virtual void clear() override;

		virtual void execute(ExecutionContext& ctx) override;
	};

	class ComputeCommand : public ShaderCommand
	{
	protected:
		
		friend struct ComputeCommandTemplateProcessor;
		
		struct MyDispatchCallInfo
		{
			using Index = ShaderCommandList::Index;
			Index name_begin = 0;
			Index pc_begin = 0;
			uint32_t name_size = 0;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			VkExtent3D extent = makeUniformExtent3D(0);
			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path shader_path;
			DynamicValue<VkExtent3D> extent = {};
			bool dispatch_threads = false;
			MultiDescriptorSetsLayouts sets_layouts = {};
			MyVector<ShaderBindingDescription> bindings = {};
			DynamicValue<DefinitionsList> definitions = DefinitionsList();
		};
		using CI = CreateInfo;

		

		struct DispatchInfo : public ShaderCommandList
		{
			bool dispatch_threads = false;
			
			MyVector<MyDispatchCallInfo> dispatch_list = {};

			struct CallInfo
			{
				std::string_view name = {};
				VkExtent3D extent = {};
				const void * pc_data = nullptr;
				uint32_t pc_size = 0;
				uint32_t pc_offset = 0;
				std::shared_ptr<DescriptorSetAndPool> set = nullptr;
			};

			void pushBack(CallInfo const& info);

			void pushBack(CallInfo && info);

			DispatchInfo& operator+=(CallInfo const& info)
			{
				pushBack(info);
				return *this;
			}

			DispatchInfo& operator+=(CallInfo&& info)
			{
				pushBack(std::move(info));
				return *this;
			}

			void clear();
		};

		using DispatchCallInfo = DispatchInfo::CallInfo;

		std::shared_ptr<ComputeCommandNode> geExecutionNodeCommon();

	protected:

		std::filesystem::path _shader_path;
		DynamicValue<DefinitionsList> _definitions;

		std::shared_ptr<ComputeProgram> _program = nullptr;
		DynamicValue<VkExtent3D> _extent = {};
		bool _dispatch_threads = false;

	public:

		ComputeCommand(CreateInfo const& ci);

		virtual ~ComputeCommand() override = default;


		virtual void init() override;

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, DispatchInfo const& di);
		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, DispatchInfo && di);

		Executable with(DispatchInfo const& di);
		Executable with(DispatchInfo && di);

		Executable operator()(DispatchInfo const& di)
		{
			return with(di);
		}

		Executable operator()(DispatchInfo && di)
		{
			return with(std::move(di));
		}

		struct SingleDispatchInfo {
			std::optional<VkExtent3D> extent = {};
			std::optional<bool> dispatch_threads = {};
			const void * pc_data = nullptr;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			// Invocation set
			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};

		Executable with(SingleDispatchInfo const& sdi);
		Executable with(SingleDispatchInfo && sdi);

		Executable operator()(SingleDispatchInfo const& sdi)
		{
			return with(sdi);
		}

		Executable operator()(SingleDispatchInfo && sdi)
		{
			return with(std::move(sdi));
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

		constexpr bool getDispatchThreads() const
		{
			return _dispatch_threads;
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