#include <thread>
#include <iterator>
#include <cmath>
#include <vector>
#include <iostream>

#include "ThreadPool.hpp"

template<class ForwardIt, class Func>
void parallel_scan(ForwardIt begin, ForwardIt end, Func func, size_t thread_count = std::thread::hardware_concurrency())
{
	using T = typename std::iterator_traits<ForwardIt>::value_type;
	auto get = [=](size_t index) -> T& { return *(begin + index); };
	auto applier = [&](size_t begin, size_t end, size_t modulo, size_t step) 
	{ 
		for (size_t index = begin; index < end; index += modulo)
		{
			get(index) = func(get(index - step), get(index));
		}
	};

	SimpleThreadPool pool(thread_count-1);	
	std::vector<Future<void>> futures;

	size_t size = std::distance(begin, end);
	for (size_t modulo = 2; modulo <= size; modulo *= 2)
	{
		size_t step = modulo / 2;

		size_t items_to_work = size / modulo;
		if (items_to_work < 4)
		{
			// trivial implementation
			applier(modulo - 1, size, modulo, step);
		}
		else
		{
			// parallel implementation
			size_t block_size = (items_to_work + 0.5 * thread_count) / thread_count;
			for (size_t index = 0; index < thread_count - 1; ++index)
			{
				size_t begin = index * block_size * modulo + modulo - 1;
				size_t end = begin + block_size * modulo;
				auto binded_applier = std::bind(applier, begin, end, modulo, step);
				futures.push_back(pool.runAsync(binded_applier));
			}

			size_t begin = (thread_count - 1) * block_size * modulo + modulo - 1;
			size_t end = size;
			if (begin < end)
			{
				applier(begin, end, modulo, step);
			}

			for (auto & fut : futures)
			{
				fut.get();
			}
			futures.clear();
		}
	}

	for (int modulo = 1 << int(log2(size)); modulo > 1; modulo /= 2)
	{
		int step = modulo / 2;

		size_t items_to_work = size / modulo;
		if (items_to_work < 4)
		{
			// trivial implementation
			applier(modulo - 1 + step, size, modulo, step);
		}
		else
		{
			// parallel implementation
			size_t block_size = (items_to_work + 0.5 * thread_count) / thread_count;
			for (size_t index = 0; index < thread_count - 1; ++index)
			{
				size_t begin = index * block_size * modulo + modulo - 1 + step;
				size_t end = std::min(begin + block_size * modulo, size);
				auto binded_applier = std::bind(applier, begin, end, modulo, step);
				futures.push_back(pool.runAsync(binded_applier));
			}

			size_t begin = (thread_count - 1) * block_size * modulo + modulo - 1 + step;
			size_t end = size;
			if (begin < end)
			{
				applier(begin, end, modulo, step);
			}

			for (auto & fut : futures)
			{
				fut.get();
			}
			futures.clear();
		}
	}
}