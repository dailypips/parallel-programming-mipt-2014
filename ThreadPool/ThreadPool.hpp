#include <vector>
#include <thread>
#include <utility>
#include <condition_variable>
#include <type_traits>

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
			workers.emplace_back(&ThreadPool::doWork, this);
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

template<class T, class Fn> 
std::function<void()> make_func(Future<T> & future, Fn & task)
{
	return [future, &task]() mutable 
	{ 
		try
		{
			future.set(task()); 
		}
		catch(const std::exception & e)
		{
			future.setException(e);
		}
	};
}

template<class Fn>
std::function<void()> make_func(Future<void> & future, Fn & task)
{
	return [future, &task]() mutable 
	{ 
		try
		{
			task(); 
			future.set(); 
		}
		catch(const std::exception & e)
		{
			future.setException(e);
		}
	};
}

template<class T>
class PriorityQueueStrategy
{
private:
	static const int DEFAULT_PRIORITY = 0;

	PriorityQueue<T> queue;

	std::mutex mutex;
	std::condition_variable condition;
	bool isClosed;	

	void addTask(const T & task, int priority)
	{
		std::unique_lock<std::mutex> lock(mutex);
		queue.add(task, priority);
		condition.notify_one();
	}

public:
	PriorityQueueStrategy() : 
		isClosed(false)
	{
	}

	~PriorityQueueStrategy()
	{
		closeQueue();
	}

	std::shared_ptr<T> getNext()
	{
		while(true)
		{
			std::unique_lock<std::mutex> lock(mutex);
			condition.wait(lock, [this]() -> bool 
			{
				return !queue.empty() || isClosed;
			});

			lock.unlock();
			if (!queue.empty())
			{
				auto ptr = queue.get_min();
				if (ptr)
				{
					return ptr;
				}
			}
			
			if (isClosed)
			{
				break;
			}
		}
		
		return std::shared_ptr<T>();
	}		

	template<class Fn>
	Future<typename std::result_of<Fn()>::type> runAsync(Fn & task, int priority = DEFAULT_PRIORITY)
	{
		Future<typename std::result_of<Fn()>::type> future;
		auto fn = make_func(future, task);

		addTask(fn, priority);
		return future;
	}

	void closeQueue()
	{
		std::unique_lock<std::mutex> lock(mutex);
		isClosed = true;
		condition.notify_all();
	}
};

template<class T>
class SimpleQueueStrategy
{
private:
	std::queue<std::shared_ptr<T>> queue;
	
	std::mutex mutex;
	std::condition_variable condition;
	bool isClosed;	

	void addTask(const T & task)
	{
		std::unique_lock<std::mutex> lock(mutex);
		queue.push(std::make_shared<T>(task));
		condition.notify_one();
	}

public:
	SimpleQueueStrategy() : 
		isClosed(false)
	{
	}

	~SimpleQueueStrategy()
	{
		closeQueue();
	}

	std::shared_ptr<T> getNext()
	{
		std::unique_lock<std::mutex> lock(mutex);
		condition.wait(lock, [this]() -> bool 
		{
			return !queue.empty() || isClosed;
		});

		if (!queue.empty())
		{
			auto ptr = queue.front();
			queue.pop();
			return ptr;
		}
		
		return std::shared_ptr<T>();
	}	

	template<class Fn>
	Future<typename std::result_of<Fn()>::type> runAsync(Fn & task)
	{
		Future<typename std::result_of<Fn()>::type> future;
		auto fn = make_func(future, task);

		addTask(fn);
		return future;
	}

	void closeQueue()
	{
		std::unique_lock<std::mutex> lock(mutex);
		isClosed = true;
		condition.notify_all();
	}
};


typedef ThreadPool<SimpleQueueStrategy> SimpleThreadPool;
typedef ThreadPool<PriorityQueueStrategy> PriorityThreadPool;