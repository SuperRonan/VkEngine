#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <shared_mutex>
#include <chrono>

#include <vkl/Core/LogOptions.hpp>

namespace vkl
{
	struct TaskPriority
	{
		size_t priority = 0;

		constexpr auto operator<=>(TaskPriority const& o) const = default;

		static constexpr TaskPriority ASAP()
		{
			return TaskPriority{
				.priority = std::numeric_limits<size_t>::max(),
			};
		}

		static constexpr TaskPriority Soon()
		{
			return TaskPriority{
				.priority = std::numeric_limits<size_t>::max() / 2,
			};
		}

		static constexpr TaskPriority WhenPossible()
		{
			return TaskPriority{
				.priority = 0,
			};
		}
	};

	class AsynchTask
	{
	public:

		using Clock = std::chrono::high_resolution_clock;

		static std::mutex s_mutex;
		
		enum class Status
		{
			Success,
			Pending,
			Running,
			Paused,
			Canceled,
			RecoverableFail,
			AbsoluteFail,
			MAX_ENUM,
		};

		enum Verbosity
		{
			None = 0,
			Low = 1,
			Medium = 2,
			High = 3,
		};

		static bool StatusIsFinish(Status status)
		{
			return (status == Status::Success) || (status == Status::AbsoluteFail) || (status == Status::Canceled);
		}

		struct ReturnType
		{
			bool success;
			bool can_retry;
			std::function<bool(void)> auto_retry_f = nullptr;
			std::string error_title = {};
			std::string error_message = {};
			std::vector<std::shared_ptr<AsynchTask>> new_tasks = {};
		};

		using LambdaType = std::function<ReturnType(void)>;

	protected:

		std::string _name = {};
		int _verbosity = 0;
		TaskPriority _priority;
		Status _status = Status::MAX_ENUM;

		LambdaType _lambda = {};

		Clock::time_point _creation_time;
		Clock::time_point _begin_time;
		Clock::duration _duration;

		bool _cancel_while_running = false;

		// Callbacks on completion?

		std::vector<std::shared_ptr<AsynchTask>> _dependencies = {};

		mutable std::shared_mutex _mutex;
		std::condition_variable_any _finish_condition;

		void cancel(bool lock_mutex, const Logger * logger = {});

	public:

		struct CreateInfo
		{
			std::string name = {};
			int verbosity = 0;
			TaskPriority priority = {};
			LambdaType lambda = nullptr;
			std::vector<std::shared_ptr<AsynchTask>> dependencies = {};
		};
		using CI = CreateInfo;

		AsynchTask(CreateInfo const& ci);

		virtual ~AsynchTask();

		std::string const& name()const
		{
			return _name;
		}

		Status getStatus() const
		{
			std::shared_lock lock(_mutex);
			return _status;
		}

		TaskPriority priority()const
		{
			std::shared_lock lock(_mutex);
			return _priority;
		}

		void cancel(const Logger * logger = {})
		{
			cancel(true, logger);
		}

		bool isCanceled()
		{
			std::shared_lock lock(_mutex);
			return _status == Status::Canceled;
		}

		void setRunning()
		{
			std::unique_lock lock(_mutex);
			_status = Status::Running;
		}

		bool isSuccess()const
		{
			return getStatus() == Status::Success;
		}

		bool isReady()const;

		bool isReadyOrSoonToBe()const;

		std::vector<std::shared_ptr<AsynchTask>> run(const Logger * logger);

		void reset();

		void wait();

		void waitIFN();

		constexpr auto duration()const
		{
			return _duration;
		}

		std::vector<std::shared_ptr<AsynchTask>> const& dependencies()const
		{
			return _dependencies;
		}

		std::vector<std::shared_ptr<AsynchTask>> & dependencies()
		{
			return _dependencies;
		}
	};
}