#pragma once

#include "AsynchTask.hpp"

#include <shared_mutex>
#include <thread>
#include <deque>
#include <atomic>

namespace vkl
{

	class DelayedTaskExecutor
	{
	protected:
		bool _log_actions;
	public:

		DelayedTaskExecutor(bool log_actions):
			_log_actions(log_actions)
		{}

		virtual ~DelayedTaskExecutor()
		{

		}
		
		virtual void pushTask(std::shared_ptr<AsynchTask> const& task) = 0;

		virtual void pushTasks(const std::shared_ptr<AsynchTask> * tasks, size_t n);

		void pushTasks(std::vector<std::shared_ptr<AsynchTask>> const& tasks)
		{
			pushTasks(tasks.data(), tasks.size());
		}

		// Returns true if all tasks were completed
		// Returns false if some task could not be launched because some dependencies could not be completed
		virtual bool waitAll() = 0;

		struct MakeInfo
		{
			bool multi_thread = true;
			size_t n_threads = 0;
			bool log_actions = true;
		};
		static DelayedTaskExecutor * MakeNew(MakeInfo const& mi);
	};

	class SingleThreadTaskExecutor : public DelayedTaskExecutor
	{
	public:

	protected:

		std::deque<std::shared_ptr<AsynchTask>> _pending_tasks;

	public:

		struct CreateInfo
		{
			bool log_actions = true;
		};
		using CI = CreateInfo;

		SingleThreadTaskExecutor(CreateInfo const& ci);

		virtual ~SingleThreadTaskExecutor() override
		{

		}

		virtual void pushTask(std::shared_ptr<AsynchTask> const& task) override;

		virtual bool waitAll() override
		{
			return _pending_tasks.empty();
		}

	};

	class ThreadPool : public DelayedTaskExecutor
	{
	public:

	protected:

		// Note: I am worried that there may be deadlocks sometimes
		// If this doesnt work as expected, I should make a simpler version with only one single mutex for the pool to avoid any deadlock
		
		struct Worker
		{
			std::thread thread;
			std::shared_ptr<AsynchTask> task = nullptr;
			std::shared_mutex mutex;

			bool runningTask();

			bool idle()
			{
				return !runningTask();
			}
		};
		std::vector<Worker*> _workers = {};
		bool _should_terminate = false;

		std::atomic<size_t> _total_waiting_tasks_counter = 0;
		std::atomic<size_t> _total_running_tasks_counter = 0;

		// Sorted highest priority to lowest
		std::deque<std::shared_ptr<AsynchTask>> _ready_tasks = {};
		// Sorted highest priority to lowest
		std::deque<std::shared_ptr<AsynchTask>> _pending_tasks = {};

		// Sorted by order of insersion (like a stack)
		std::vector<std::shared_ptr<AsynchTask>> _just_pushed_tasks = {};

		//mutable std::condition_variable _wait_all;

		mutable std::condition_variable _aquire_task_condition;
		mutable std::mutex _ready_mutex;
		mutable std::shared_mutex _pending_mutex;
		mutable std::shared_mutex _just_pushed_mutex;

		// External mutex
		void insertSortedTask(std::deque<std::shared_ptr<AsynchTask>> & queue, std::shared_ptr<AsynchTask> const& task);

		
		void insertJustPushedTasks(bool can_lock_just_pushed, bool can_lock_pending, bool can_lock_ready);

		
		void transferPendingTasks(bool can_lock_pending, bool can_lock_ready);

		
		std::shared_ptr<AsynchTask> aquireTaskIFP(bool can_lock_ready);

		void threadLoop(Worker * worker);

	public:

		struct CreateInfo
		{
			size_t n = 0;
			bool log_actions = true;
		};
		using CI = CreateInfo;

		ThreadPool(CreateInfo const& ci);

		virtual ~ThreadPool() override;

		virtual void pushTask(std::shared_ptr<AsynchTask> const& task) override;

		virtual void pushTasks(const std::shared_ptr<AsynchTask> * tasks, size_t n) override;

		virtual bool waitAll() override;

	};
} // namespace vkl
