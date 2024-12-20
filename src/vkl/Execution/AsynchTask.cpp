#include <vkl/Execution/AsynchTask.hpp>

#include <vkl/Core/VulkanCommons.hpp>

#include <vkl/Execution/MessagePopUp.hpp>

#include <cassert>
#include <algorithm>
#include <iostream>

#include <Windows.h>

namespace vkl
{
	std::mutex AsynchTask::s_mutex;


	AsynchTask::AsynchTask(CreateInfo const& ci) :
		_name(ci.name),
		_verbosity(ci.verbosity),
		_priority(ci.priority),
		_status(Status::Pending),
		_lambda(ci.lambda),
		_dependencies(ci.dependencies)
	{
		_creation_time = Clock::now();
	}

	AsynchTask::~AsynchTask()
	{

	}

	bool AsynchTask::isReady()const
	{
		std::shared_lock lock(_mutex);
		bool res = _status == Status::Pending;
		if (res)
		{
			for (const auto& dep : _dependencies)
			{
				Status dep_status = dep->getStatus();
				bool dep_finish = StatusIsFinish(dep_status);
				res &= dep_finish;
			}
		}
		return res;
	}

	bool AsynchTask::isReadyOrSoonToBe()const
	{
		std::shared_lock lock(_mutex);
		bool res = _status == Status::Pending;
		if (res)
		{
			for (const auto& dep : _dependencies)
			{
				Status dep_status = dep->getStatus();
				bool dep_finish = StatusIsFinish(dep_status) || dep_status == Status::Running;
				res &= dep_finish;
			}
		}
		return res;
	}

	void AsynchTask::cancel(bool lock_mutex, const Logger * logger)
	{
		if (lock_mutex)
		{
			_mutex.lock();
		}



		if (_status == Status::Running)
		{
			_cancel_while_running = true;
		}
		else if (!StatusIsFinish(_status))
		{
			if (logger && logger->canLog(_verbosity))
			{
				logger->log(std::format("Canceling task: {}", name()), Logger::Options::TagWarning | _verbosity);
			}

			_status = Status::Canceled;
			_finish_condition.notify_all();
		}

		if (lock_mutex)
		{
			_mutex.unlock();
		}
	}

	void AsynchTask::reset()
	{
		waitIFN();
		std::unique_lock lock(_mutex);
		_status = Status::Pending;
		_creation_time = Clock::now();
		_cancel_while_running = false;
		_dependencies.clear();
	}

	std::vector<std::shared_ptr<AsynchTask>> AsynchTask::run(const Logger * logger)
	{
		assert(_lambda);
		Status prev_satus = _status;
		setRunning();
		_begin_time = Clock::now();
		const bool can_log = logger && logger->canLog(_verbosity);

		bool all_success = std::all_of(_dependencies.begin(), _dependencies.end(), [](std::shared_ptr<AsynchTask> const& dep) {
			return dep->getStatus() == Status::Success;
		});

		if (!all_success || isCanceled())
		{
			std::unique_lock lock(_mutex);
			// Cancel
			if (can_log)
			{
				logger->log(std::format("Canceling task: {}", name()), Logger::Options::TagWarning | _verbosity);
			}

			_status = Status::Canceled;
			_finish_condition.notify_all();
			return {};
		}

		bool try_run = true;

		std::vector<std::shared_ptr<AsynchTask>> new_tasks = {};

		bool common_mutex_locked = false;
		while (try_run)
		{
			if (_status == Status::Canceled || _cancel_while_running)
			{
				std::unique_lock lock(_mutex);
				_status = Status::Canceled;
				break;
			}
			ReturnType res;
			try
			{
				if (can_log)
				{
					logger->log(std::format("Launching Task: {}", name()), Logger::Options::TagInfo | _verbosity);
				}
				res = _lambda();
			}
			catch (std::exception const& e)
			{
				_status = Status::AbsoluteFail;
				res.success = false;
				res.can_retry = false;
				res.error_title = _name;
				res.error_message = "Exception: \n" + std::string(e.what());
			}

			std::unique_lock lock(_mutex);
			if (res.success)
			{
				_status = Status::Success;
				new_tasks = std::move(res.new_tasks);
				break;
			}
			else
			{
				if (!common_mutex_locked)
				{
					// Lock until the problem is resolved
					s_mutex.lock();
					common_mutex_locked = true;
				}
				if (res.can_retry)
				{
					if (res.auto_retry_f)
					{
						if (res.auto_retry_f())
						{
							assert(common_mutex_locked);
							s_mutex.unlock();
							common_mutex_locked = false;
							continue;
						}
					}
					using enum MessagePopUp::Button;
					SynchMessagePopUp popup = SynchMessagePopUp::CI{
						.type = MessagePopUp::Type::Error,
						.title = res.error_title,
						.message = res.error_message,
						.buttons = {Retry, Cancel},
						.beep = true,
						.logger = logger,
					};
					MessagePopUp::Button selected_button = popup(false);

					//MessageBeep(MB_ICONERROR);
					//int selected_option = MessageBoxA(nullptr, res.error_title.c_str(), res.error_message.c_str(), MB_ICONERROR | MB_SETFOREGROUND | MB_RETRYCANCEL);
					
					switch (selected_button)
					{
						case Retry:
						{
							continue;
						}
						break;
						case Cancel:
						{
							_status = Status::AbsoluteFail;
							try_run = false;
						}
						break;
					}
				}
				else
				{
					using enum MessagePopUp::Button;
					SynchMessagePopUp popup = SynchMessagePopUp::CI{
						.type = MessagePopUp::Type::Error,
						.title = res.error_title,
						.message = res.error_message,
						.buttons = {Ok},
						.beep = true,
						.logger = logger,
					};
					popup(false);
					//MessageBeep(MB_ICONERROR);
					//MessageBoxA(nullptr, res.error_title.c_str(), res.error_message.c_str(), MB_ICONERROR | MB_SETFOREGROUND | MB_OK);
					_status = Status::AbsoluteFail;
					try_run = false;
				}
			}
		}

		if (common_mutex_locked)
		{
			s_mutex.unlock();
			common_mutex_locked = false;
		}

		std::chrono::time_point<Clock> finish_time = Clock::now();
		_duration = finish_time - _begin_time;
		std::unique_lock lock(_mutex);
		assert(StatusIsFinish(_status));
		_finish_condition.notify_all();
		return new_tasks;
	}

	void AsynchTask::wait()
	{
		while (true)
		{
			std::unique_lock lock(_mutex);
			_finish_condition.wait(lock, [this]() -> bool {
				return StatusIsFinish(_status);
			});

			if (StatusIsFinish(_status))
			{
				break;
			}
		}
	}

	void AsynchTask::waitIFN()
	{
		std::shared_lock lock(_mutex);

		if (!StatusIsFinish(_status))
		{
			lock.unlock();
			wait();
		}
	}
}