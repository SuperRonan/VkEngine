#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include "FileSystem.hpp"

#include <vkl/Execution/ThreadPool.hpp>

namespace vkl
{
	class DependencyTracker
	{
	public:

		using Path = FileSystem::Path;
		using PathString = FileSystem::PathString;
		using PathStringView = FileSystem::PathStringView;
		using TimePoint = FileSystem::TimePoint;
		using Duration = TimePoint::duration;
		using Clock = FileSystem::Clock;

		// callback(last_write_time)
		using FileCallback = GenericCallback<TimePoint, that::Result>;

	protected:

		FileSystem* _fs;
		DelayedTaskExecutor* _executor;
		const Logger * _log;

		Duration _period;
		TimePoint _latest_launch;

		struct BackValue
		{
			that::Result result;
			bool registered;
			TimePoint last_write_time;
		};

		// used by the back thread, might be out of synch
		std::unordered_map<PathString, BackValue> _back_files;

		std::shared_mutex _mutex;
		struct Registration
		{
			std::unordered_map<uintptr_t, FileCallback::Function> callbacks;
		};
		// always in synch
		std::unordered_map<PathString, Registration> _registered;
		that::ExtensibleBasicStringContainer<PathString::value_type, uint32_t> _back_registered;

		struct CheckResult
		{
			that::ResultAnd<TimePoint> time;
			Range32u range; // points to path
		};
		that::ExtensibleBasicStringStorage<PathString::value_type> _check_results_paths;
		MyVector<CheckResult> _check_results;

		std::shared_ptr<AsynchTask> _check_task;

		bool _task_is_running = false;
		bool _registration_update = false;

		friend class DependencyTrackerHelper;
	
	public:

		struct CreateInfo
		{
			FileSystem* file_system = nullptr;
			DelayedTaskExecutor* executor = nullptr;
			const Logger* log = nullptr;
			Duration period = 1s;
		};
		using CI = CreateInfo;

		DependencyTracker(CreateInfo const& ci);

		~DependencyTracker();

		void setDependency(PathString const& cannon_file_path, FileCallback const& cb);

		void removeDependency(PathString const& cannon_file_path, FileCallback::Id id);

		// Returns whether checks were broadcasted
		bool update();
	};
}