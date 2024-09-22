#pragma once

#include <vkl/Commands/ShaderCommand.hpp>
#include <vkl/Execution/ShaderBindingTable.hpp>

namespace vkl
{
	class RayTracingCommand : public ShaderCommand
	{
	public:

	protected:

		friend struct RayTracingCommandTemplateProcessor;
		
		Dyn<DefinitionsList> _common_shader_definitions;

		Dyn<VkExtent3D> _extent;
		std::shared_ptr<ShaderBindingTable> _sbt = nullptr;

		struct MyTraceCallInfo
		{
			using Index = ShaderCommandList::Index;
			Index name_begin = 0;
			Index pc_begin = 0;
			uint32_t name_size = 0;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			VkExtent3D extent = {};
			ShaderBindingTable* sbt = nullptr;
			std::shared_ptr<DescriptorSetAndPool> set = {};
			VkDeviceSize stack_size = VkDeviceSize(-1);
		};

	public:

		struct RTShader
		{
			std::filesystem::path path = {};
			DefinitionsList definitions = {};
		};

		struct HitGroup
		{
			uint32_t closest_hit = VK_SHADER_UNUSED_KHR;
			uint32_t any_hit = VK_SHADER_UNUSED_KHR;
			uint32_t intersection = VK_SHADER_UNUSED_KHR;
		};

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			RTShader raygen = {};
			MyVector<RTShader> misses;
			MyVector<RTShader> closest_hits;
			MyVector<RTShader> any_hits;
			MyVector<RTShader> intersections;
			MyVector<RTShader> callables;
			MyVector<HitGroup> hit_groups;
			Dyn<DefinitionsList> definitions;
			MyVector<ShaderBindingDescription> bindings = {};
			Dyn<VkExtent3D> extent = {};
			Dyn<uint32_t> max_recursion_depth = {};
			bool create_sbt = false;
			VkBufferUsageFlags sbt_extra_buffer_usage = 0;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		RayTracingCommand(CreateInfo const& ci);

		virtual ~RayTracingCommand() override;

		std::shared_ptr<RayTracingPipeline> getRTPipeline()const
		{
			return std::static_pointer_cast<RayTracingPipeline>(_pipeline);
		}

		// The caller MUST call sbt->recordUpdate(ctx);
		std::shared_ptr<ShaderBindingTable> const& getSBT()const
		{
			return _sbt;
		}

		struct TraceInfo : public ShaderCommandList
		{
			MyVector<MyTraceCallInfo> calls;

			ShaderBindingTable * sbt = nullptr;
			VkDeviceSize stack_size = VkDeviceSize(-1);

			struct CallInfo
			{
				std::string_view name = {};
				ShaderBindingTable * sbt = nullptr;
				VkExtent3D extent = {};
				const void * pc_data = nullptr;
				uint32_t pc_size = 0;
				uint32_t pc_offset = 0;
				std::shared_ptr<DescriptorSetAndPool> set = {};
				VkDeviceSize stack_size = VkDeviceSize(-1);
			};

			void pushBack(CallInfo const& ci);
			void pushBack(CallInfo && ci);

			TraceInfo& operator+=(CallInfo const& ci)
			{
				pushBack(ci);
				return *this;
			}

			TraceInfo& operator+=(CallInfo && ci)
			{
				pushBack(std::move(ci));
				return *this;
			}

			void clear();
		};
		using TraceCallInfo = TraceInfo::CallInfo;

		struct SingleTraceInfo
		{
			std::optional<VkExtent3D> extent = {};
			const void * pc_data = nullptr;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			ShaderBindingTable * sbt = nullptr;
			std::shared_ptr<DescriptorSetAndPool> set = {};
			VkDeviceSize stack_size = VkDeviceSize(-1);
		};

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, TraceInfo const& ti);
		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, TraceInfo && ti);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

		Executable with(TraceInfo const& ti);
		Executable with(TraceInfo && ti);

		Executable with(SingleTraceInfo const& sti);
		Executable with(SingleTraceInfo && sti);

		Executable operator()(TraceInfo const& ti)
		{
			return with(ti);
		}

		Executable operator()(TraceInfo && ti)
		{
			return with(std::move(ti));
		}

		Executable operator()(SingleTraceInfo const& sti)
		{
			return with(sti);
		}

		Executable operator()(SingleTraceInfo && sti)
		{
			return with(std::move(sti));
		}

		virtual bool updateResources(UpdateContext & ctx) override;
	};
}