#include <iostream>
#include <mutex>
#include <vector>
#include <boost/thread.hpp>
#include <future>

#include "HierarchicalMutex.h"

#define BOOST_TEST_MODULE HierMutexTest
#include <boost/test/included/unit_test.hpp>

void success()
{
	HierarchicalMutex mutex1(101);
	HierarchicalMutex mutex2(100);
	typedef std::lock_guard<HierarchicalMutex> lg;
	
	auto f1 = [&]()
	{
		lg lk1(mutex1);
		lg lk2(mutex2);
	};

	auto f2 = [&]()
	{
		lg lk2(mutex1);
		lg lk1(mutex2);
	};

	std::vector<boost::thread> threads;
	for (size_t index = 0; index < 5; ++index)
	{
		threads.emplace_back(f1);
		threads.emplace_back(f2);
	}

	for (auto & it : threads)
	{
		it.join();
	}
}

void fail()
{
	HierarchicalMutex mutex1(100);
	HierarchicalMutex mutex2(100);
	typedef std::lock_guard<HierarchicalMutex> lg;

	auto f1 = [&]()
	{
		lg lk1(mutex1);
		lg lk2(mutex2);
	};

	auto f2 = [&]()
	{
		lg lk2(mutex2);
		lg lk1(mutex1);
	};

	std::vector<std::future<void>> futures;
	for (size_t index = 0; index < 5; ++index)
	{
		futures.push_back(std::async(f1));
		futures.push_back(std::async(f2));
	}

	for (auto & it : futures)
	{
		it.get();
	}
}

BOOST_AUTO_TEST_CASE(success_test)
{
	BOOST_CHECK_NO_THROW(success());
}

BOOST_AUTO_TEST_CASE(fail_test)
{
	BOOST_CHECK_THROW(fail(), std::logic_error);
}
/*
int main()
{
	HierarchicalMutex mutex1(100);
	HierarchicalMutex mutex2(500);
	HierarchicalMutex mutex3(600);
	typedef std::unique_lock<HierarchicalMutex> lg;
	lg lk1(mutex2);
	lg lk2(mutex1);

	lk1.unlock();
	lk2.unlock();

	lg lk3(mutex3);

	return 0;
}*/
