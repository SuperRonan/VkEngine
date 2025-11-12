#pragma once

#include <vkl/Execution/ResourcesLists.hpp>
#include <vkl/App/VkApplication.hpp>
#include <vkl/Execution/UpdateContext.hpp>
#include <chrono>

namespace vkl
{
	class ResourcesManager : public VkObject
	{
	protected:

		size_t _update_tick = 0;

		std::chrono::milliseconds _auto_file_check_period;

		// TODO separate device life-time constant definitions
		// and other possibly non const definitions
		std::unique_ptr<DefinitionsMap> _common_definitions;

		UploadQueue _upload_queue;
		MipMapComputeQueue _mips_queue;

		DescriptorWriter _descriptor_writer;

		void populateCommonObjects();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::chrono::milliseconds auto_file_check_period = 1000ms;
		};

		ResourcesManager(CreateInfo const& ci);

		virtual ~ResourcesManager();

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
	};
}