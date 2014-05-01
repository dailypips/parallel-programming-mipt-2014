#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <boost/thread.hpp>

#include "TLSLib.hpp"

#define BOOST_TEST_MODULE TLSLIB_TEST
#ifdef _WIN32
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif

void print_sizes()
{
	std::cout << sizeof(tls::super_block_t) << std::endl;
}

class timer
{
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> begin;
	std::string message;
public:
	timer(const std::string & message) :
		message(message)
	{
		begin = std::chrono::high_resolution_clock::now();
	}

	~timer()
	{
		auto end = std::chrono::high_resolution_clock::now();
		auto dura = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
		std::cout << message << ": " << dura.count() << " ms\n";
	}
};

struct point
{
	int x[1000];
};

struct obj2
{
	int x, y;
};

void allocating_func()
{
	std::vector<obj2 *> vec;

	if (true)
	{
		//timer t("allocating");
		for (size_t index = 0; index < 10000; ++index)
		{
			vec.push_back(new obj2());
		}
	}

	if (true)
	{
		//timer t("deleting");
		for (size_t index = 0; index < 10000; ++index)
		{
			delete vec[index];
		}
	}
}

BOOST_AUTO_TEST_CASE(big_object_test)
{
	point * p = new point();
	delete p;
}

BOOST_AUTO_TEST_CASE(many_small_objects_test)
{
	std::vector<boost::thread> threads;
	{
		timer t("allocating and freeing");
		for(size_t index = 0; index < 4; ++index)
		{
			threads.emplace_back(&allocating_func);
		}

		for(auto & it: threads)
		{
			it.join();
		}
	}
}
