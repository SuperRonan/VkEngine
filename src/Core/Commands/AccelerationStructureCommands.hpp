#pragma once

#include "DeviceCommand.hpp"
#include <Core/VkObjects/AccelerationStructure.hpp>
#include <Core/Execution/BufferPool.hpp>

namespace vkl
{
	
	class BuildAccelerationStructureCommand : public DeviceCommand
	{
	public:

	protected:

		BufferPool _scratch_buffer_pool;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		BuildAccelerationStructureCommand(CreateInfo const& ci);

		virtual ~BuildAccelerationStructureCommand() override;

		struct BuildInfo
		{
			struct Target
			{
				std::shared_ptr<AccelerationStructure> src = nullptr;
				std::shared_ptr<AccelerationStructure> dst = nullptr;
				uint32_t range_offset = uint32_t(-1);
				VkBuildAccelerationStructureModeKHR mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
			};
			MyVector<Target> targets;
			MyVector<VkAccelerationStructureBuildRangeInfoKHR> ranges;

			BufferAndRangeInstance scratch_buffer;
			
			void clear();

			bool pushIFN(std::shared_ptr<AccelerationStructure> const& as);

			bool empty() const
			{
				return targets.empty();
			}

			//size_t push(std::shared_ptr<AccelerationStructure> const& as, size_t range_count); // TODO
		};

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, BuildInfo const& bi);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override
		{
			NOT_YET_IMPLEMENTED;
			return nullptr;
		}

		Executable with(BuildInfo const& bi);

		Executable operator()(BuildInfo const& bi)
		{
			return with(bi);
		}
	};
}