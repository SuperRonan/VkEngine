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

		size_t _update_cycle = 0;

		size_t _shader_check_cycle = 0;
		using _shader_clock_t = std::chrono::system_clock;
		std::chrono::time_point<_shader_clock_t> _last_shader_check;
		std::chrono::milliseconds _shader_check_period;

		const DefinitionsMap * _common_definitions;

		MountingPoints * _mounting_points = nullptr;

		UploadQueue * _upload_queue = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::chrono::milliseconds shader_check_period = 1000ms;
			const DefinitionsMap * common_definitions;
			MountingPoints * mounting_points = nullptr;
			UploadQueue * upload_queue = nullptr;
		};

		ResourcesManager(CreateInfo const& ci);


		std::shared_ptr<UpdateContext>  beginUpdateCycle();

		void finishUpdateCycle(std::shared_ptr<UpdateContext> context);

	};
}