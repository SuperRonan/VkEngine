#pragma once

#include <vector>
#include <string>
#include "DefinitionMap.hpp"
#include <Core/Commands/ShaderBindingDescriptor.hpp>
#include <Core/Execution/ResourcesLists.hpp>
#include <Core/Execution/ResourcesToUpload.hpp>
#include <Core/Execution/UploadQueue.hpp>
#include <Core/Execution/FramePerformanceCounters.hpp>

#include <Core/Utils/TickTock.hpp>

namespace vkl
{
	class UpdateContext : public VkObject
	{
	protected:

		size_t _update_cycle;

		size_t _shader_check_cycle;

		bool _update_resources_anyway = false;
		
		const DefinitionsMap * _common_definitions;


		const MountingPoints* _mounting_points = nullptr;

		ResourcesLists _resources_to_update_later;

		// Synchronous upload
		ResourcesToUpload _resources_to_upload;
		
		UploadQueue * _upload_queue;

		std::TickTock_hrc _tick_tock;

		FramePerfCounters * _frame_perf_counters = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			size_t update_cycle = 0;
			size_t shader_check_cycle = 0;
			const DefinitionsMap* common_definitions;
			MountingPoints* mounting_points = nullptr;
			UploadQueue * upload_queue = nullptr;
		};
		using CI = CreateInfo;

		UpdateContext(CreateInfo const& ci) :
			VkObject(ci.app, ci.name),
			_update_cycle(ci.update_cycle),
			_shader_check_cycle(ci.shader_check_cycle),
			_common_definitions(ci.common_definitions),
			_mounting_points(ci.mounting_points),
			_upload_queue(ci.upload_queue)
		{
			_tick_tock.tick();
		}

		constexpr size_t updateCycle()const
		{
			return _update_cycle;
		}

		constexpr size_t checkShadersCycle() const 
		{ 
			return _shader_check_cycle; 
		}

		constexpr const DefinitionsMap * commonDefinitions() const
		{
			return _common_definitions;
		}

		const MountingPoints* mountingPoints()
		{
			return _mounting_points;
		}

		const MountingPoints* mountingPoints() const
		{
			return _mounting_points;
		}

		ResourcesLists& resourcesToUpdateLater()
		{
			return _resources_to_update_later;
		}

		ResourcesToUpload& resourcesToUpload()
		{
			return _resources_to_upload;
		}

		constexpr bool updateAnyway() const
		{
			return _update_resources_anyway;
		}

		constexpr UploadQueue* uploadQueue()
		{
			return _upload_queue;
		}

		constexpr auto& tickTock()
		{
			return _tick_tock;
		}

		FramePerfCounters* getFramePerfCounters()const
		{
			return _frame_perf_counters;
		}

		void setFramePerfCounters(FramePerfCounters* pfc)
		{
			_frame_perf_counters = pfc;
		}
	};
}