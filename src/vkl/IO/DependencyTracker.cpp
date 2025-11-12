#include <vkl/IO/DependencyTracker.hpp>

namespace vkl
{
	struct DependencyTrackerHelper
	{
		static void CheckFiles(DependencyTracker& that)
		{
			that._check_results_paths.clear();
			that._check_results.clear();

			// update cache from _back_registered
			{
				for (auto& [k, v] : that._back_files)
				{
					v.registered = false;
				}

				DependencyTracker::PathString path;
				for (size_t i = 0; i < that._back_registered.size(); ++i)
				{
					DependencyTracker::PathStringView sv = that._back_registered[i];
					path = sv;
					if (!that._back_files.contains(path))
					{
						// MAX_ENUM means no relevant value
						that._back_files.insert({ path, DependencyTracker::BackValue{
							.result = that::Result::MAX_ENUM,
							.registered = true,
						} });
					}
					else
					{
						that._back_files.at(path).registered = true;
					}
				}

				auto it = that._back_files.begin();
				while (it != that._back_files.end())
				{
					if (!it->second.registered)
					{
						it = that._back_files.erase(it);
					}
					else
					{
						++it;
					}
				}
			}

			{
				DependencyTracker::Path path;
				for (auto& [file, cache] : that._back_files)
				{
					path = file;
					auto last_write_time = that._fs->getFileLastWriteTime(path, FileSystem::Hint::QueryCache);
					bool new_value = false;
					// if MAX_ENUM, last_write_time not set yet
					if (cache.result != that::Result::MAX_ENUM)
					{
						if (last_write_time.result != cache.result)
						{
							new_value = true;
						}
						else if (last_write_time.result == that::Result::Success && last_write_time.value != cache.last_write_time)
						{
							new_value = true;
						}
					}

					cache.result = last_write_time.result;
					cache.last_write_time = last_write_time.value;
					if (new_value)
					{
						Range32u range = {.begin = static_cast<uint32_t>(that._check_results_paths.size()), .len = static_cast<uint32_t>(file.size()) };
						that._check_results_paths.pushBack(file);
						that._check_results.push_back(DependencyTracker::CheckResult{
							.time = last_write_time,
							.range = range,
						});
					}
				}
			}
		}
	};

	DependencyTracker::DependencyTracker(CreateInfo const& ci) :
		_fs(ci.file_system),
		_executor(ci.executor),
		_period(ci.period)
	{
		_latest_launch = Clock::now() - 2 * _period;

		_check_task = std::make_shared<AsynchTask>(AsynchTask::CI{
			.name = "Auto Check file dependencies",
			.verbosity = 3,
			.priority = TaskPriority::WhenPossible(),
			.lambda = [this]() {
				DependencyTrackerHelper::CheckFiles(*this);
				AsynchTask::ReturnType res;
				res.success = true;
				res.can_retry = false;
				return res;
			},
		});
	}

	void DependencyTracker::setDependency(PathString const& cannon_file_path, FileCallback const& cb)
	{
		_mutex.lock();
		_registration_update |= !_registered.contains(cannon_file_path);
		_registered[cannon_file_path].callbacks[reinterpret_cast<uintptr_t>(cb.id)] = cb.callback;
		_mutex.unlock();
	}

	void DependencyTracker::removeDependency(PathString const& cannon_file_path, FileCallback::Id id)
	{
		_mutex.lock();
		if (_registered.contains(cannon_file_path))
		{
			Registration& reg = _registered.at(cannon_file_path);
			size_t erased = reg.callbacks.erase(reinterpret_cast<uintptr_t>(id));
			if (reg.callbacks.empty())
			{
				_registered.erase(cannon_file_path);
				_registration_update |= true;
			}
		}
		_mutex.unlock();
	}

	bool DependencyTracker::update()
	{
		bool res = false;

		if (_check_task->isSuccess())
		{
			_task_is_running = false;
			if (_check_results)
			{
				_mutex.lock_shared();
				PathString path;
				for (size_t i = 0; i < _check_results.size(); ++i)
				{
					CheckResult const& cr = _check_results[i];
					path = _check_results_paths.get(cr.range);
					if (_registered.contains(path))
					{
						const auto& registration = _registered.at(path);
						for (const auto& [id, cb] : registration.callbacks)
						{
							cb(cr.time.value, cr.time.result);
						}
					}
				}
				_mutex.unlock_shared();
			}
			_check_task->reset();
			res = true;
		}

		bool launch_checks = !_task_is_running;
		TimePoint now;
		if (launch_checks)
		{
			now = Clock::now();
			launch_checks = now - _latest_launch > _period;
		}
		if (launch_checks)
		{
			if (_registration_update)
			{
				_mutex.lock_shared();
				_back_registered.clear();
				for (const auto& [path, _] : _registered)
				{
					_back_registered.pushBack(path);
				}
				_mutex.unlock_shared();
				_registration_update = false;
			}

			_task_is_running = true;
			_latest_launch = now;
			_executor->pushTask(_check_task);
		}

		return res;
	}
}