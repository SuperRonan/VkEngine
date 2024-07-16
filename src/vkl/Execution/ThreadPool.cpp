#include <vkl/Execution/ThreadPool.hpp>
#include <cassert>
#include <algorithm>

#include <iostream>

namespace vkl
{

	DelayedTaskExecutor* DelayedTaskExecutor::MakeNew(MakeInfo const& mi)
	{
		DelayedTaskExecutor* res = nullptr;
		if (mi.multi_thread)
		{
			res = new ThreadPool(ThreadPool::CI{
				.n = mi.n_threads,
				.log_level = mi.log_level,
			});
		}
		else
		{
			res = new SingleThreadTaskExecutor(SingleThreadTaskExecutor::CI{
				.log_level = mi.log_level,
			});
		}
		return res;
	}

	void DelayedTaskExecutor::pushTasks(const std::shared_ptr<AsynchTask>* tasks, size_t n)
	{
		for (size_t i = 0; i < n; ++i)
		{
			pushTask(tasks[i]);
		}
	}










	SingleThreadTaskExecutor::SingleThreadTaskExecutor(CreateInfo const& ci) :
		DelayedTaskExecutor(ci.log_level)
	{

	}

	void SingleThreadTaskExecutor::pushTask(std::shared_ptr<AsynchTask> const& task)
	{
		assert(!!task);
		if (task->isReady())
		{
			task->run(_log_level);
			while (true)
			{
				bool none_ready = true;
				auto it = _pending_tasks.begin();
				while(it != _pending_tasks.end())
				{
					std::shared_ptr<AsynchTask> it_task = *it;
					if (it_task->isReady())
					{
						std::vector<std::shared_ptr<AsynchTask>> new_tasks = it_task->run(_log_level);
						it = _pending_tasks.erase(it);
						
						for (std::shared_ptr<AsynchTask> ntsk : new_tasks)
						{
							it = _pending_tasks.insert(it, ntsk);
						}

						none_ready = false;
					}
					else
					{
						++it;
					}
				}

				if (none_ready)
				{
					break;
				}
			}
		}
		else
		{
			_pending_tasks.push_back(task);
		}
	}












	ThreadPool::ThreadPool(CreateInfo const& ci) :
		DelayedTaskExecutor(ci.log_level)
	{
		size_t n = ci.n;
		if (n == 0)
		{
			n = std::thread::hardware_concurrency();
		}

		_workers.resize(n);
		for (size_t i = 0; i < n; ++i)
		{
			_workers[i] = new Worker;
			_workers[i]->thread = std::thread(&ThreadPool::threadLoop, this, _workers[i]);
		}
	}

	bool ThreadPool::Worker::runningTask()
	{
		std::shared_lock lock(mutex);
		return !!task;
	}

	void ThreadPool::threadLoop(Worker * worker)
	{
		while (true)
		{
			std::unique_lock lock(_ready_mutex);
			_aquire_task_condition.wait(lock, [this]() -> bool {
				return _total_waiting_tasks_counter.load() > 0 || _should_terminate;
			});
			if(_should_terminate)
			{
				return;
			}
			std::shared_ptr<AsynchTask> task = aquireTaskIFP(false);
			lock.unlock();

			std::vector<std::shared_ptr<AsynchTask>> new_tasks = {};
			if (task)
			{
				worker->mutex.lock();
				worker->task = task;
				worker->mutex.unlock();

				new_tasks = task->run(_log_level);
				
				worker->mutex.lock();
				worker->task = nullptr;
				worker->mutex.unlock();
				_total_running_tasks_counter.fetch_sub(1);
			}
			transferPendingTasks(true, true);
			if (!new_tasks.empty())
			{
				pushTasks(new_tasks.data(), new_tasks.size());
			}
			insertJustPushedTasks(true, true, true);
		}
	}

	void ThreadPool::insertSortedTask(std::deque<std::shared_ptr<AsynchTask>>& queue, std::shared_ptr<AsynchTask> const& task)
	{
		if (queue.empty())
		{
			queue.push_front(task);
		}
		else
		{
			auto it = queue.begin();
			while (it != queue.end())
			{
				const std::shared_ptr<AsynchTask>& it_task = *it;
				if (task->priority() <= it_task->priority())
				{
					++it;
				}
				else
				{
					break;
				}
			}
			queue.insert(it, task);
		}
	}

	void ThreadPool::transferPendingTasks(bool can_lock_pending, bool can_lock_ready)
	{
		if (can_lock_pending)
		{
			_pending_mutex.lock();
		}
		auto it = _pending_tasks.begin();
		while(it != _pending_tasks.end())
		{
			std::shared_ptr<AsynchTask> task = *it;
			if (task->isReady())
			{
				
				if (can_lock_ready)
				{
					_ready_mutex.lock();
				}
				insertSortedTask(_ready_tasks, task);
				if (can_lock_ready)
				{
					_ready_mutex.unlock();
				}
				it = _pending_tasks.erase(it);

				_aquire_task_condition.notify_one();
			}
			else
			{
				++it;
			}
		}
		if (can_lock_pending)
		{
			_pending_mutex.unlock();
		}
	}

	void ThreadPool::insertJustPushedTasks(bool can_lock_just_pushed, bool can_lock_pending, bool can_lock_ready)
	{
		std::vector<std::shared_ptr<AsynchTask>> just_pushed_tasks = [&](){
			if (can_lock_just_pushed)
			{
				_just_pushed_mutex.lock();
			}
			std::vector<std::shared_ptr<AsynchTask>> tmp = _just_pushed_tasks;
			_just_pushed_tasks.clear();
			if (can_lock_just_pushed)
			{
				_just_pushed_mutex.unlock();
			}
			return tmp;
		}();


		for (std::shared_ptr<AsynchTask> const& task : just_pushed_tasks)
		{
			if (task->isReady())
			{
				if (can_lock_ready)
				{
					_ready_mutex.lock();
				}
				insertSortedTask(_ready_tasks, task);
				if (can_lock_ready)
				{
					_ready_mutex.unlock();
				}
				_aquire_task_condition.notify_one();
			}
			else
			{
				if (can_lock_pending)
				{
					_pending_mutex.lock();
				}
				insertSortedTask(_pending_tasks, task);
				if (can_lock_pending)
				{
					_pending_mutex.unlock();
				}
			}
		}
	}

	std::shared_ptr<AsynchTask> ThreadPool::aquireTaskIFP(bool can_lock_ready)
	{
		if (can_lock_ready)
		{
			_ready_mutex.lock();
		}
		std::shared_ptr<AsynchTask> res;
		while(!_ready_tasks.empty())
		{
			res = std::move(_ready_tasks.front());
			_ready_tasks.pop_front();
			if (res->isCanceled())
			{
				res = nullptr;
			}
			else
			{
				break;
			}
		}

		if(res)
		{
			_total_running_tasks_counter.fetch_add(1);
			_total_waiting_tasks_counter.fetch_sub(1);
		}
		
		if (can_lock_ready)
		{
			_ready_mutex.unlock();
		}
		return res;
	}

	void ThreadPool::pushTask(std::shared_ptr<AsynchTask> const& task)
	{
		assert(!!task);
		std::unique_lock lock(_just_pushed_mutex);
		_total_waiting_tasks_counter.fetch_add(1);
		_just_pushed_tasks.push_back(task);
		_aquire_task_condition.notify_one();
	}

	void ThreadPool::pushTasks(const std::shared_ptr<AsynchTask>* tasks, size_t n)
	{
		std::unique_lock lock(_just_pushed_mutex);
		_total_waiting_tasks_counter.fetch_add(1);
		_just_pushed_tasks.reserve(_just_pushed_tasks.size() + n);
		// Notify one every loop, or notify all at the end
		for (size_t i = 0; i < n; ++i)
		{
			std::shared_ptr<AsynchTask> ntsk = tasks[i];
			_just_pushed_tasks.push_back(ntsk);
			_aquire_task_condition.notify_one();
		}
	}

	bool ThreadPool::waitAll()
	{
		// TODO Block pushTask
		// Just this would create a deadlock with threadLoop
		//std::unique_lock push_task_lock(_just_pushed_mutex);
		
		insertJustPushedTasks(true, true, true);

		bool need_to_wait_more = true;

		const int default_tries = 4;
		int tries = default_tries;

		while (need_to_wait_more)
		{
			size_t total_running_tasks_counter = _total_running_tasks_counter.load();
			if (total_running_tasks_counter == 0)
			{
				std::unique_lock ready_lock(_ready_mutex);
				std::shared_lock pending_lock(_pending_mutex);
				std::shared_lock just_pushed_lock(_just_pushed_mutex);

				if (_ready_tasks.empty())
				{
					const bool any_ready = std::any_of(_pending_tasks.begin(), _pending_tasks.end(), [](std::shared_ptr<AsynchTask> const& t){return t->isReadyOrSoonToBe();});
					
					if (!any_ready)
					{
						need_to_wait_more = false;
					}
				}
			}

			if (!need_to_wait_more)
			{
				--tries;
				if (tries)
				{
					need_to_wait_more = true;
				}
			}
			else
			{
				tries = default_tries;
			}

			if (need_to_wait_more)
			{
				//std::this_thread::yield();
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(1ms);
			}
		}

		//while (true)
		//{
		//	
		//	bool all_idle = [&]()
		//	{
		//		bool _res = true;
		//		for (Worker* worker : _workers)
		//		{
		//			_res &= worker->idle();
		//		}
		//		return _res;
		//	}();

		//	std::this_thread::yield();
		//}

		std::unique_lock ready_lock(_ready_mutex);
		std::shared_lock pending_lock(_pending_mutex);
		std::shared_lock just_pushed_lock(_just_pushed_mutex);

		assert(_ready_tasks.empty());
		bool res = _pending_tasks.empty();

		return res;
	}


	ThreadPool::~ThreadPool()
	{
		_should_terminate = true;
		_aquire_task_condition.notify_all();

		{
			std::unique_lock lock(_ready_mutex);
			for (auto& task : _ready_tasks)
			{
				task->cancel(_log_level);
			}
		}

		{
			std::unique_lock lock(_pending_mutex);
			for (auto& task : _pending_tasks)
			{
				task->cancel(_log_level);
			}
		}

		for (Worker* & worker : _workers)
		{
			worker->thread.join();
			assert(!worker->runningTask());
			delete worker;
			worker = nullptr;
		}
	}
	

} // namespace vkl
