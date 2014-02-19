#include <vector>
#include <thread>
#include <utility>
#include <condition_variable>

#include "ThreadsafePriorityQueue.hpp"

template<template<class K> class QUEUE_STRATEGY>
// QUEUE_STRATEGY: 
// Future<R> runAsync(T task, ...)
// std::shared_ptr<T> getNext();
// void closeQueue();
class ThreadPool : public QUEUE_STRATEGY<std::function<void()>>
{
private:
	std::vector<std::thread> workers;

	void doWork()
	{
		while(true)
		{
			auto task = getNext();
			if (!task)
			{
				break;
			}

			(*task)();
		}
	}

public:
	ThreadPool()
	{
		unsigned int core_count = std::thread::hardware_concurrency();
		for(size_t index = 0; index < core_count; ++index)
		{
			workers.emplace_back(std::bind(&ThreadPool::doWork, this));
		}
	}

	~ThreadPool()
	{
		close();
		for(auto & it : workers)
		{
			it.join();
		}
	}

	void close()
	{
		closeQueue();
	}
};

typedef ThreadPool<SimpleQueueStrategy> SimpleThreadPool;
typedef ThreadPool<PriorityQueueStrategy> PriorityThreadPool;