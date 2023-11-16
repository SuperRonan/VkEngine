#include "AsynchTask.hpp"

#include <Core/VulkanCommons.hpp>

#include <Core/Execution/MessagePopUp.hpp>

#include <cassert>
#include <algorithm>
#include <iostream>

#include <Windows.h>

namespace vkl
{
	
	AsynchTask::AsynchTask(CreateInfo const& ci) :
		_name(ci.name),
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

	void AsynchTask::cancel(bool lock_mutex, bool verbose)
	{
		if (lock_mutex)
		{
			_mutex.lock();
		}
		
		if (!StatusIsFinish(_status))
		{
			if (verbose)
			{
				std::unique_lock lock(g_mutex);
				std::cout << "Canceling task: " << name() << std::endl;
			}

			_status = Status::Canceled;
			_finish_condition.notify_all();
		}

		if (lock_mutex)
		{
			_mutex.unlock();
		}
	}

	std::vector<std::shared_ptr<AsynchTask>> AsynchTask::run(bool verbose)
	{
		assert(_lambda);
		setRunning();
		_begin_time = Clock::now();

		bool all_success = std::all_of(_dependencies.begin(), _dependencies.end(), [](std::shared_ptr<AsynchTask> const& dep) {
			return dep->getStatus() == Status::Success;
		});

		if (!all_success)
		{
			cancel(true, verbose);
			return {};
		}

		bool try_run = true;

		std::vector<std::shared_ptr<AsynchTask>> new_tasks = {};

		while (try_run)
		{
			ReturnType res;
			try
			{
				if (verbose)
				{
					std::unique_lock lock(g_mutex);
					std::cout << "Launching task: " << name() << std::endl;
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
				if (res.can_retry)
				{
					using enum MessagePopUp::Button;
					SynchMessagePopUp popup = SynchMessagePopUp::CI{
						.type = MessagePopUp::Type::Error,
						.title = res.error_title,
						.message = res.error_message,
						.buttons = {Retry, Cancel},
						.beep = true,
						.log_cout = true,
					};
					MessagePopUp::Button selected_button = popup();

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
						.log_cout = true,
					};
					popup();
					//MessageBeep(MB_ICONERROR);
					//MessageBoxA(nullptr, res.error_title.c_str(), res.error_message.c_str(), MB_ICONERROR | MB_SETFOREGROUND | MB_OK);
					_status = Status::AbsoluteFail;
					try_run = false;
				}
			}
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