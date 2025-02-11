
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

#include "helper.hpp"

#include <vkl/Maths/Transforms.hpp>

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


void StressTestParallelDynamicValue(int count = 1024)
{
	using namespace vkl;
	const auto fill = [&](DefinitionsList& res)
	{
		for (int i = 0; i < count; ++i)
		{
			res += GetString(i);
		}
	};

	Dyn<DefinitionsList> dv;
	dv = [&]()
	{
		DefinitionsList res;
		fill(res);
		return res;
	};

	dv = [&](DefinitionsList& res)
	{
		res.clear();
		fill(res);
	};

	auto thread_main = [&dv]()
	{
		const DefinitionsList & l = dv.value();

		std::unique_lock lock(g_common_mutex);
		
		for (size_t i = 0; i < l.size(); ++i)
		{
			std::cout << l.at(i) << "\n";
		}
	};

	const uint n = 2;//std::thread::hardware_concurrency();
	MyVector<std::jthread> threads;
	for (uint i = 0; i < n; ++i)
	{
		threads.push_back(std::jthread(thread_main));
	}


	for (uint i = 0; i < n; ++i)
	{
		threads[i].join();
	}
}


int main(int argc, const char** argv)
{
	using namespace vkl;
	using namespace std::containers_append_operators;
	using namespace std::string_literals;

	using namespace vkl;

	Matrix3f Rx = Rotation3X(Radians(60.0f));
	Matrix3f Ry = Rotation3Y(Radians(60.0f));
	Matrix3f Rz = Rotation3Z(Radians(60.0f));

	std::cout << std::setprecision(3);

	//std::cout << glm::MultiLinePrint(Rx) << std::endl << std::endl;
	//std::cout << glm::MultiLinePrint(Ry) << std::endl << std::endl;
	//std::cout << glm::MultiLinePrint(Rz) << std::endl << std::endl;

	//std::cout << glm::MultiLinePrint(BasisFromDir_hughes_moeller(Vector3f(1, 0, 0))) << std::endl << std::endl;
	//std::cout << glm::MultiLinePrint(BasisFromDir_hughes_moeller(Vector3f(0, 1, 0))) << std::endl << std::endl;
	//std::cout << glm::MultiLinePrint(BasisFromDir_hughes_moeller(Vector3f(0, 0, 1))) << std::endl << std::endl;

	//StressTestParallelDynamicValue(4096);

	//TestHalf();

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

	pii += 12;

	pii.value();

	BindingIndex b{
		.set = 2,
		.binding = 3,
	};
	b.asString();
	std::cout << b;

	using T = typename BinaryOperator_spaceship<int, double>::Type;

	auto cmp = pii <=> tau;
	cmp.value();

	Dyn<BindingIndex> db = b;

	db->binding;

	// TODO make these lines work
	// From the compiler error, int the DynValue<T>'s operator +, it doesn't find T's operator + for std::vector
	// My operators are not even mentioned, so it doesn't seem to be a concept failure issue
	// My Container's operator + isn't even in the list of candidates it appears...
	{
		Dyn<MyVector<std::string>> s = {{"a"s, "b"s}};
		Dyn<MyVector<std::string>> t = s + "c"s;

		Dyn<Array<int>> vec = Array<int>{12, 23};
		Dyn<Array<int>> vec2 = vec + vec;
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

