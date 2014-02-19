#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <future>

#include "ThreadPool.hpp"

#define THREAD_COUNT 100
#define ELEM_COUNT 100

void threadFunc(PriorityQueue<int> & queue)
{
	for(size_t index = 0; index < ELEM_COUNT; ++index)
	{
		queue.add(index, 0);
	}
}

void priorityQueueTest()
{
	PriorityQueue<int> queue;
	std::vector<std::thread> threads;
	for(size_t index = 0; index < THREAD_COUNT; ++index)
	{
		threads.emplace_back(&threadFunc, std::ref(queue));
	}

	for(size_t index = 0; index < THREAD_COUNT * ELEM_COUNT; ++index)
	{
		std::cout << *queue.get_min() << std::endl;
	}

	for(auto & it : threads)
	{
		if (it.joinable())
		{
			it.join();
		}
	}
}

int main()
{
	PriorityThreadPool pool;

	auto f = []() { 
		std::cout << "hello" << std::endl; 
		return 0; 
	};

	auto f1 = []() { std::cout << "here's a void function" << std::endl; 
		std::chrono::seconds dura(3);
		std::this_thread::sleep_for(dura); };

	auto future = pool.runAsync<int>(f);
	auto future1 = pool.runAsync<void>(f1);

	std::cout << future.get() << std::endl;
	future1.get();

	system("pause");

	return 0;
}

