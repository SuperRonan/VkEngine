#pragma once

#include <Core/Execution/ResourcesLists.hpp>
#include <Core/App/VkApplication.hpp>
#include <Core/Execution/UpdateContext.hpp>
#include <chrono>

namespace vkl
{
	class ResourcesManager : public VkObject
	{
	protected:

		size_t _update_tick = 0;

		size_t _shader_check_tick = 0;
		using _shader_clock_t = std::chrono::system_clock;
		std::chrono::time_point<_shader_clock_t> _last_shader_check;
		std::chrono::milliseconds _shader_check_period;

		std::unique_ptr<DefinitionsMap> _common_definitions;

		std::unique_ptr<MountingPoints> _mounting_points;

		UploadQueue _upload_queue;
		MipMapComputeQueue _mips_queue;

		DescriptorWriter _descriptor_writer;

		void populateCommonObjects();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::chrono::milliseconds shader_check_period = 1000ms;
		};

		ResourcesManager(CreateInfo const& ci);


		std::shared_ptr<UpdateContext>  beginUpdateCycle();

		void finishUpdateCycle(std::shared_ptr<UpdateContext> context);


		UploadQueue& uploadQueue()
		{
			return _upload_queue;
		}

		MipMapComputeQueue& mipMapQueue()
		{
			return _mips_queue;
		}

		DefinitionsMap* commonDefinitions()const
		{
			return _common_definitions.get();
		}

		MountingPoints* mountingPoints()const
		{
			return _mounting_points.get();
		}
	};
}