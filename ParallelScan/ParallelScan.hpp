#include <thread>
#include <iterator>
#include <cmath>
#include <vector>
#include <iostream>

#include "ThreadPool.hpp"

template<class ForwardIt, class Func>
void parallel_scan(ForwardIt begin, ForwardIt end, Func func, size_t threadCount = std::thread::hardware_concurrency())
{
	using T = typename std::iterator_traits<ForwardIt>::value_type;
	auto get = [=](size_t index) -> T& { return *(begin + index); };
	auto applier = [&](size_t index, size_t step) 
	{ 
		get(index) = func(get(index - step), get(index)); 
	};

	SimpleThreadPool pool(threadCount);	
	std::vector<Future<void>> futures;

	size_t size = std::distance(begin, end);
	for (int modulo = 2; modulo <= size; modulo *= 2)
	{
		int step = modulo / 2;
		for (int index = modulo - 1; index < size; index += modulo)
		{
			auto bindedApplier = std::bind(applier, index, step);
			futures.push_back(pool.runAsync(bindedApplier));
		}

		for (auto & fut : futures)
		{
			fut.get();
		}
		futures.clear();
	}

	for (int modulo = 1 << int(log2(size)); modulo > 1; modulo /= 2)
	{
		int step = modulo / 2;
		for (int index = modulo - 1 + step; index < size; index += modulo)
		{
			auto bindedApplier = std::bind(applier, index, step);
			futures.push_back(pool.runAsync(bindedApplier));
		}

		for (auto & fut : futures)
		{
			fut.get();
		}
		futures.clear();
	}
}