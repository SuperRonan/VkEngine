#pragma once

#include <vector>
#include <string>
#include "DefinitionMap.hpp"
#include <vkl/Execution/ResourcesLists.hpp>
#include <vkl/Execution/ResourcesToUpload.hpp>
#include <vkl/Execution/UploadQueue.hpp>
#include <vkl/Execution/FramePerformanceCounters.hpp>
#include <vkl/Execution/DescriptorWriter.hpp>

#include <vkl/Utils/TickTock.hpp>

namespace vkl
{
	class UpdateContext : public VkObject
	{
	protected:

		size_t _update_tick;

		bool _update_resources_anyway = false;
		
		const DefinitionsMap * _common_definitions;

		ResourcesLists _resources_to_update_later;

		// Synchronous upload
		ResourcesToUpload _resources_to_upload;
		
		UploadQueue * _upload_queue = nullptr;
		MipMapComputeQueue * _mips_queue = nullptr;

		std::TickTock_hrc _tick_tock;

		FramePerfCounters * _frame_perf_counters = nullptr;

		DescriptorWriter & _descriptor_writer;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			size_t update_tick = 0;
			const DefinitionsMap* common_definitions;
			UploadQueue * upload_queue = nullptr;
			MipMapComputeQueue * mips_queue = nullptr;
			DescriptorWriter& descriptor_writer;
		};
		using CI = CreateInfo;

		UpdateContext(CreateInfo const& ci) :
			VkObject(ci.app, ci.name),
			_update_tick(ci.update_tick),
			_common_definitions(ci.common_definitions),
			_upload_queue(ci.upload_queue),
			_mips_queue(ci.mips_queue),
			_descriptor_writer(ci.descriptor_writer)
		{
			_tick_tock.tick();
		}

		constexpr size_t updateTick()const
		{
			return _update_tick;
		}

		constexpr const DefinitionsMap * commonDefinitions() const
		{
			return _common_definitions;
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

		constexpr MipMapComputeQueue* mipsQueue()
		{
			return _mips_queue;
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

		DescriptorWriter& descriptorWriter()
		{
			return _descriptor_writer;
		}
	};
}