
#include <Core/DynamicValue.hpp>
#include <Core/VulkanCommons.hpp>
#include <Core/Utils/stl_extension.hpp>

#include <chrono>
#include <thread>
#include <iostream>

#include <Core/Execution/ThreadPool.hpp>


template <class T, Container<T> C>
const T* f(C const& c)
{
	return c.data();
}

int main(int argc, const char** argv)
{
	using namespace vkl;
	using namespace std::containers_operators;
	using namespace std::string_literals;

	dv_<VkExtent3D> ex = makeZeroExtent3D();

	dv_<float> pi = 3.14f;

	dv_<int> pii = pi;

	dv_<double> tau = pi + pii;



	std::vector<std::string> a = { "a" };
	std::vector<std::string> bc = { "b", "c"};

	f<std::string>(a);

	std::vector<std::string> abc = a + bc;

	std::vector d = abc + "d"s;

	std::vector e = d + "e"s;


	Dyn<std::vector<std::string>> s = {{"a"s, "b"s}};
	// TODO make this line work
	//Dyn<std::vector<std::string>> t = s + "c"s;

	//std::chrono::time_point<std::chrono::high_resolution_clock> tic = std::chrono::high_resolution_clock::now();
	//
	//auto show_message = [&]()
	//	{
	//		MessageBeep(MB_ICONERROR);
	//		int res = MessageBoxA(nullptr, "Message", "Caption", MB_RETRYCANCEL | MB_ICONERROR);
	//		std::cout << res << std::endl;
	//		return res;
	//	};
	//std::jthread thread = std::jthread(show_message);
	//std::chrono::time_point<std::chrono::high_resolution_clock> toc = std::chrono::high_resolution_clock::now();	
	//std::chrono::duration delay = toc - tic;
	//std::this_thread::sleep_for(2s);
	//std::cout << "Cancel thread" << std::endl;
	//thread.request_stop();
	//int end_res;
	//
	//thread.join();
	//thread.join();
	//std::this_thread::sleep_for(2s);


	DelayedTaskExecutor* pool = DelayedTaskExecutor::MakeNew(DelayedTaskExecutor::MakeInfo{
		.multi_thread = true,
		.n_threads = 0,
	});

	int task_value = 0;

	std::shared_ptr<AsynchTask> t1 = std::make_shared<AsynchTask>(AsynchTask::CI{
		.name = "Task 1",
		.priority = TaskPriority::ASAP(),
		.lambda = [&]() {
			std::this_thread::sleep_for(2s);
			task_value = 1;
			return AsynchTask::ReturnType{
				.success = true,
			};
		},
	});

	std::shared_ptr<AsynchTask> t2 = std::make_shared<AsynchTask>(AsynchTask::CI{
		.name = "Task 2",
		.priority = TaskPriority::ASAP(),
		.lambda = [&]() {
			std::this_thread::sleep_for(3s);
			
			std::shared_ptr<AsynchTask> t25 = std::make_shared<AsynchTask>(AsynchTask::CI{
				.name = "Task 2.5",
				.priority = TaskPriority{.priority = 0},
				.lambda = [&task_value]() {
					std::this_thread::sleep_for(2.5s);
					task_value = 25;
					return AsynchTask::ReturnType{
						.success = true,
					};
				},
			});
			std::vector<std::shared_ptr<AsynchTask>> new_tasks = {t25};
			
			task_value = 2;
			return AsynchTask::ReturnType{
				.success = true,
				.new_tasks = new_tasks,
			};
		},
		.dependencies = {t1},
	});

	std::shared_ptr<AsynchTask> t3 = std::make_shared<AsynchTask>(AsynchTask::CI{
		.name = "Task 3",
		.priority = TaskPriority::ASAP(),
		.lambda = [&]() {
			std::this_thread::sleep_for(512ms);

			if ((rand() % 2) == 0)
			{
				task_value = 3;
				return AsynchTask::ReturnType{
					.success = true,
				};
			}
			else
			{
				return AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Task 3",
					.error_message = "You can retry",
				};
			}
		},
		.dependencies = {t1},
	});

	std::shared_ptr<AsynchTask> t4 = std::make_shared<AsynchTask>(AsynchTask::CI{
		.name = "Task 4",
		.priority = TaskPriority::ASAP(),
		.lambda = [&]() {
			std::this_thread::sleep_for(4s);
			task_value = 4;
			return AsynchTask::ReturnType{
				.success = true,
			};
		},
		.dependencies = {t2, t3},
	});

	pool->pushTask(t3);
	pool->pushTask(t2);
	pool->pushTask(t1);
	pool->pushTask(t4);

	t4->wait();

	std::cout << "WaitAll" << std::endl;
	pool->waitAll();

	delete pool;
	pool = nullptr;

	std::cout << task_value << std::endl;

	return 0;
}

