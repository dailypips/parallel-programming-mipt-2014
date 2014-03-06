#pragma once
#include <queue>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/thread.hpp>

#include "Future.hpp"

template<class T>
class PriorityQueue
{
private:
	struct QueueItem
	{
		std::shared_ptr<T> item;
		int priority;

		QueueItem(const T & item, int priority)
		{
			this->item = std::make_shared<T>(item);
			this->priority = priority;
		}

		bool operator<(const QueueItem & other) const
		{
			return priority < other.priority;
		}
	};

	typedef boost::detail::spinlock SpinLock;

	std::vector<std::pair<QueueItem, SpinLock>> heap;
	mutable boost::shared_mutex readWriteLock;

	typedef boost::shared_lock<boost::shared_mutex> ReadLock;
	typedef boost::unique_lock<boost::shared_mutex> WriteLock;

	std::unique_lock<SpinLock> getLock(size_t index, bool isDefer = false)
	{
		if (isDefer)
		{
			return std::unique_lock<boost::detail::spinlock>(heap[index].second, std::defer_lock);
		}
		else
		{
			return std::unique_lock<boost::detail::spinlock>(heap[index].second);
		}
	}

	size_t size() const
	{
		return heap.size();
	}

	size_t min(size_t first, size_t second) const
	{
		if (heap[first].first < heap[second].first)
		{
			return first;
		}

		return second;
	}

	bool compareAndSwap(size_t first, size_t second)
	{
		if (heap[second].first < heap[first].first)
		{
			std::swap(heap[first].first, heap[second].first);
			return true;
		}
		return false;
	}

	void siftDown(size_t index, std::unique_lock<SpinLock> currentLock)
	{
		if (index >= size())
		{
			return;
		}

		size_t left = 2 * index + 1;
		size_t right = 2 * index + 2;

		if (size() < right)
		{
			// we have no children
			return;
		}

		auto leftLock = getLock(left, true);
		std::unique_lock<boost::detail::spinlock> rightLock;
		if (size() > right)
		{
			// two children
			rightLock = getLock(right, true);
			std::lock(leftLock, rightLock);
		}
		else
		{
			// one child
			leftLock.lock();
		}

		int swapIndex = -1;
		if (size() == right)
		{
			swapIndex = left;
		}
		else
		{
			swapIndex = min(left, right);
		}

		bool needHeapify = compareAndSwap(index, swapIndex);

		if (!needHeapify)
		{
			return;
		}

		currentLock.unlock();

		if (size() > right)
		{
			// two children
			if (swapIndex == left)
			{
				rightLock.unlock();
				siftDown(left, std::move(leftLock));
			}
			else
			{
				leftLock.unlock();
				siftDown(right, std::move(rightLock));
			}
		}
		else
		{
			siftDown(left, std::move(leftLock));
		}
	}

	void siftUp(size_t index)
	{
		if (index > 0)
		{
			size_t parent = (index - 1) / 2;

			if (compareAndSwap(parent, index))
			{
				siftUp(parent);
			}
		}
	}

public:
	std::shared_ptr<T> getMin()
	{
		boost::upgrade_lock<boost::shared_mutex> lock(readWriteLock);
		std::shared_ptr<T> result;		

		if (size() == 0)
		{
			return std::shared_ptr<T>();
		}

		auto topLock = getLock(0, true); 

		if (size() == 1)
		{
			topLock.lock();
			boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
			result = heap.front().first.item;
			heap.pop_back();
			return result;
		}

		auto lastLock = getLock(size() - 1, true);
		std::lock(topLock, lastLock);
		{
			boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
			result = heap.front().first.item;
			std::swap(heap.front().first, heap.back().first);
			heap.pop_back();
		}

		lastLock.unlock();
		siftDown(0, std::move(topLock));
		return result;
	}

	void add(const T & task, int priority)
	{
		WriteLock lock(readWriteLock);
		heap.emplace_back(QueueItem(task, priority), SpinLock());
		size_t lastIndex = size() - 1;
		siftUp(lastIndex);
	}

	bool empty() const
	{
		ReadLock lock(readWriteLock);
		return heap.size() == 0;
	}
};