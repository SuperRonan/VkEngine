
#define SDL_MAIN_HANDLED

#include <vkl/Utils/stl_extension.hpp>
#include <vkl/Core/DynamicValue.hpp>
#include <vkl/Core/VulkanCommons.hpp>

#include <chrono>
#include <thread>
#include <iostream>

#include <vkl/Execution/ThreadPool.hpp>

#include <vkl/Utils/UniqueIndexAllocator.hpp>
#include <random>

#include <that/math/Half.hpp>

void TestUniqueIdAllocator()
{
	vkl::UniqueIndexAllocator pool(vkl::UniqueIndexAllocator::Policy::FitCapacity);

	while (true)
	{
		int i;
		std::cin >> i;
		
		if (i >= 0)
		{
			if (pool.isAllocated(i))
			{
				pool.release(i);
			}
			else
			{
				std::cout << "Can't release an unallocated index" << std::endl;
			}
		}
		else
		{
			i = pool.allocate();
			std::cout << "Allocated " << i << std::endl;
		}

		pool.print(std::cout);
		std::cout << std::endl;
	}
}

void TestHalf()
{
	using float16_t = that::math::float16_t;
	std::mt19937_64 rng;
	auto generate = [&]()
	{
		float r = 16;
		std::uniform_real_distribution<float> d (-r, r);
		float f = d(rng);
		return pow(f, 4);
	};

	float max_dr = 0;

	for (int i = 0; i < 1024 * 1024; ++i)
	{
		float f = generate();
		float16_t h = f;
		float hf = h;
		float d = hf - f;
		float dr = d * 100 / f;
		max_dr = std::max(abs(dr), max_dr);
		if(i / 1024 == 0)
		{
			std::cout << std::hex << f << " -> " << hf << " (" << d << ", " << dr << "%)" << std::endl;
		}
	}
	VKL_BREAKPOINT_HANDLE;
}

template <class T, std::concepts::Container<T> C>
const T* f(C const& c)
{
	return c.data();
}

template <class Op, class L, class R = L>
concept BinaryOpWrapperConcept = requires(Op op, L l, R r)
{
	op(l, r);
};

template <class L, class R = L>
struct MyPlus
{
	decltype(auto) operator()(L&& l, R&& r) const
	{
		return std::forward<L>(l) + std::forward<R>(r);
	}

	decltype(auto) operator()(const L & l, const R & r) const
	{
		return l + r;
	}
};

template <class L, class R, class Op>
concept CombinableWithOperator = BinaryOpWrapperConcept<Op, L, R>;

static_assert(CombinableWithOperator<int, float, MyPlus<int, float>>);

//static_assert(std::concepts::GenericContainer<vkl::OptVector<int>>);

int main(int argc, const char** argv)
{
	using namespace vkl;
	using namespace std::containers_append_operators;
	using namespace std::string_literals;

	TestHalf();

	Dyn<VkExtent3D> ex = makeUniformExtent3D(0);

	Dyn<float> pi = 3.14f;

	Dyn<int> pii = pi;

	Dyn<double> tau = pi + pii + 0.14;

	Dyn<std::string> str = "abcdef";
	Dyn<std::string> rts = "fedcba";
	Dyn concat = str + rts;

	Dyn<MyVector<int>> dyn_list = [](MyVector<int>& res)
	{
		res.clear();
		res.push_back(rand());
	};

	std::vector<std::string> a = { "a" };
	std::vector<std::string> bc = { "b", "c"};

	f<std::string>(a);

	std::vector<std::string> abc = a + bc;

	std::vector d = abc + "d"s;

	std::vector e = d + "e"s;


		
	// TODO make these lines work
	// From the compiler error, int the DynValue<T>'s operator +, it doesn't find T's operator + for std::vector
	// My operators are not even mentioned, so it doesn't seem to be a concept failure issue
	// My Container's operator + isn't even in the list of candidates it appears...
	{
		Dyn<std::vector<std::string>> s = {{"a"s, "b"s}};
		//Dyn<std::vector<std::string>> t = s + "c"s;

		Dyn<Array<int>> vec = Array<int>{12, 23};
		//Dyn<Array<int>> vec2 = vec + vec;
	}

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

