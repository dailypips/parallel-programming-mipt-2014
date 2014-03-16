#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <future>

#include "ThreadPool.hpp"

class Matrix
{
private:
	size_t size;
	std::vector<int> matrix;
public:
	Matrix(size_t size) :
		size(size),
		matrix(size * size, 0)
	{
	}

	int & at(size_t row, size_t column)
	{
		return matrix[row * size + column];
	}

	void fill()
	{
		for (size_t index = 0; index < size * size; ++index)
		{
			matrix[index] = rand() % 10;
		}
	}

	void print()
	{
		std::cout << "======================================" << std::endl;

		for (size_t index = 0; index < size * size; ++index)
		{
			std::cout << matrix[index] << " ";
			if ((index + 1) % size == 0) 
			{
				std::cout << std::endl;
			}
		}

		std::cout << "======================================" << std::endl;
	}
};

void queue_test()
{
	std::cout << "starting queue test" << std::endl;
	PriorityQueue<int> queue;

	const int OPERATIONS_PER_THREAD = 500;
	const int THREADS_COUNT = 20;

	auto filler = [&]()
	{
		for (size_t index = 0; index < OPERATIONS_PER_THREAD; ++index)
		{
			queue.add(index, index);
		}
	};

	auto getter = [&]()
	{
		for (size_t index = 0; index < OPERATIONS_PER_THREAD; ++index)
		{
			queue.getMin();
		}
	};

	auto worker = [=](std::function<void()> func)
	{
		std::vector<boost::thread> threads;
		for (size_t index = 0; index < THREADS_COUNT; ++index)
		{
			threads.emplace_back(func);
		}

		for (auto & it : threads)
		{
			it.join();
		}
	};

	std::cout << "start filling" << std::endl;
	worker(filler);
	std::cout << "size after filling: " << queue.size() << std::endl;
	std::cout << "start getting" << std::endl;
	worker(getter);
	std::cout << "size after getting: " << queue.size() << std::endl;
	std::cout << "done" << std::endl;

	assert(queue.empty());
}

int main()
{
	queue_test();

	PriorityThreadPool pool;
	
	std::cout << "start matrix test" << std::endl;

	srand(time(NULL));
	const int size = 1000;
	Matrix m1(size), m2(size), m3(size);
	std::vector<Future<void>> futures;

	std::cout << "begin fill" << std::endl;

	m1.fill();
	m2.fill();

	std::cout << "end fill" << std::endl;

	auto f = [&](size_t row)
	{
		for (size_t column = 0; column < size; ++column)
		{
			for (size_t elem = 0; elem < size; ++elem)
			{
				m3.at(row, column) += m1.at(row, elem) * m2.at(elem, column);
			}
		}
	};

	auto begin = std::chrono::high_resolution_clock::now();

	for (size_t index = 0; index < size; ++index)
	{
		futures.push_back(pool.runAsync(std::bind(f, index)));
	}

	std::for_each(futures.begin(), futures.end(), std::mem_fn(&Future<void>::get));

	auto end = std::chrono::high_resolution_clock::now();
	auto tm = std::chrono::duration_cast<std::chrono::duration<double>>(end - begin);
	std::cout << tm.count() << std::endl;

	return 0;
}