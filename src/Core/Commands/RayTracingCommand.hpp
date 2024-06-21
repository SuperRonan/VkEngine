#pragma once

#include <Core/Commands/ShaderCommand.hpp>
#include <Core/Execution/ShaderBindingTable.hpp>

namespace vkl
{
	class RayTracingCommand : public ShaderCommand
	{
	public:

	protected:
		
		Dyn<DefinitionsList> _common_shader_definitions;

		Dyn<VkExtent3D> _extent;
		std::shared_ptr<ShaderBindingTable> _sbt = nullptr;

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
			std::vector<ShaderBindingDescription> bindings = {};
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

		struct TraceInfo
		{
			std::string name = {};
			ShaderBindingTable * sbt = nullptr;
			VkExtent3D extent = {};
			PushConstant pc = {};
			std::shared_ptr<DescriptorSetAndPool> set = {};
			VkDeviceSize stack_size = VkDeviceSize(-1);
		};

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, size_t n, const TraceInfo * ti);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

		Executable with(TraceInfo const& ti);

		Executable operator()(TraceInfo const& ti)
		{
			return with(ti);
		}

		virtual bool updateResources(UpdateContext & ctx) override;
	};
}