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

	std::vector<std::pair<QueueItem, boost::detail::spinlock>> heap;
	mutable boost::shared_mutex readWriteLock;

	typedef boost::shared_lock<boost::shared_mutex> ReadLock;
	typedef boost::unique_lock<boost::shared_mutex> WriteLock;

	std::unique_lock<boost::detail::spinlock> getLock(size_t index, bool isDefer = false)
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

	void heapify(size_t index)
	{
		ReadLock readLock(readWriteLock);

		if (index >= size())
		{
			return;
		}

		// sift up
		if (index > 0)
		{
			size_t parent = (index-1) / 2;

			auto currentLock = getLock(index, true);
			auto parentLock = getLock(parent, true);
			std::lock(currentLock, parentLock);			

			if (compareAndSwap(parent, index))
			{
				currentLock.unlock();
				parentLock.unlock();
				readLock.unlock();
				heapify(parent);
				return;
			}
		}

		size_t left = 2 * index + 1;
		size_t right = 2 * index + 2;

		if (size() < right)
		{
			// we have no children
			return;
		}

		auto currentLock = getLock(index, true);
		auto leftLock = getLock(left, true);
		std::unique_lock<boost::detail::spinlock> rightLock;
		if (size() > right) 
		{
			rightLock = getLock(right, true);
			std::lock(currentLock, leftLock, rightLock);
		}
		else
		{
			std::lock(currentLock, leftLock);
		}

		int swap_index = -1;
		if (size() == right)
		{
			swap_index = left;
		}
		else
		{
			swap_index = min(left, right);
		}

		bool needHeapify = compareAndSwap(index, swap_index);

		currentLock.unlock();
		leftLock.unlock();
		if (size() > right)
		{
			rightLock.unlock();
		}

		if (needHeapify)
		{
			readLock.unlock();
			heapify(swap_index);
		}
	}	

	size_t size() const
	{
		return heap.size();
	}

public:
	std::shared_ptr<T> get_min()
	{
		WriteLock writeLock(readWriteLock);

		if (heap.empty())
		{
			return std::shared_ptr<T>();
		}

		auto min_ptr = heap.front().first.item;
		std::swap(heap.front().first, heap.back().first);
		heap.pop_back();

		if (size() < 2)
		{
			return min_ptr;
		}

		if (size() == 2)
		{
			compareAndSwap(0, 1);
			return min_ptr;
		}

		size_t min_child = min(1, 2);
		if (compareAndSwap(0, min_child))
		{
			writeLock.unlock();
			heapify(min_child);
			return min_ptr;
		}

		return min_ptr;
	}

	bool empty() const
	{
		ReadLock lock(readWriteLock);
		return heap.size() == 0;
	}

	void add(const T & value, int priority)
	{
		WriteLock writelock(readWriteLock);
		heap.emplace_back(QueueItem(value, priority), boost::detail::spinlock());
		size_t lastIndex = size() - 1;
		writelock.unlock();
		heapify(lastIndex);
	}
};