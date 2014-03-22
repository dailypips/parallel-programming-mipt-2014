#include <iostream>
#include <vector>
#include <thread>
#include <fstream>

#include "Logger.h"

int main()
{
	Logger log(std::cout);

	const int items = 10000;
	const int threadsCount = 10;

	auto f = [&](int order)
	{
		for (size_t index = 0; index < items; ++index)
		{
			log << order * items + index;
		}
	};

	std::vector<std::thread> threads;

	for (size_t index = 0; index < threadsCount; ++index)
	{
		threads.emplace_back(f, index);
	}

	for (auto & it : threads)
	{
		it.join();
	}
	return 0;
}