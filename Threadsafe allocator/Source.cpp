#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <condition_variable>
#include <mutex>
#include <boost/thread.hpp>

#include "TLSLib.hpp"

#define BOOST_TEST_MODULE TLSLIB_TEST
#ifdef _WIN32
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif

#define D(a) std::cout << a << std::endl;

template<class TIME_UNIT>
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
		auto dura = std::chrono::duration_cast<TIME_UNIT>(end - begin);
		D(message << ": " << dura.count() << " ms");
	}
};

typedef timer<std::chrono::milliseconds> milli_timer;
typedef timer<std::chrono::microseconds> micro_timer;

template<class FUNC>
void starter(FUNC func, size_t thread_count, const std::string & message)
{
	std::vector<boost::thread> threads;
	
	milli_timer t(message);
	for(size_t index = 0; index < thread_count; ++index)
	{
		threads.emplace_back(func);
	}

	for(auto & it: threads)
	{
		it.join();
	}
}

struct separator_printer
{
	~separator_printer()
	{
		D("\n==========================================\n");
	}
};

typedef separator_printer sp;

template<size_t SIZE>
struct object
{
	char data[SIZE];
};

typedef object<8> small;
typedef object<1000> big;

void allocating_func()
{
	std::vector<small *> vec;

	const size_t items_to_test = 500;
	const size_t test_count = 50;

	for(size_t test = 0; test < test_count; ++test)
	{
		for (size_t index = 0; index < items_to_test; ++index)
		{
			vec.push_back(new small);
		}

		for(auto & it: vec)
		{
			delete it;
		}
		vec.clear();
	}
}

BOOST_AUTO_TEST_CASE(memory_drift_test)
{
	sp p;
	D("memory drift test");
	std::vector<small *> objs;
	std::atomic_int t;

	t.store(0);

	const size_t items_to_test = 1000;

	auto f1 = [&]()
	{	
		for(size_t index = 0; index < items_to_test; ++index)
		{
			objs.push_back(new small);
		}
		D("objects initialized");
		t++;
		while(t.load() != 2)
			;
		small * m = new small;
		delete m;
	};

	auto f2 = [&]()
	{
		D("waiting for objects initialization");
		while(t.load() != 1)
			;
		for(auto & it: objs)
		{
			delete it;			
		}

		D("successfully deleted in other thread");
		t++;
	};

	boost::thread t1(f1);
	boost::thread t2(f2);
	t1.join();
	t2.join();
}

BOOST_AUTO_TEST_CASE(big_object_test)
{
	sp pr;
	D("allocating object of size " << sizeof(big));
	big * p = new big();
	delete p;
}

BOOST_AUTO_TEST_CASE(many_small_objects_test)
{
	sp p;
	const size_t thread_count = 4;

	starter(&allocating_func, thread_count, "allocating and freeing");
}

BOOST_AUTO_TEST_CASE(diff_sizes_test)
{
	sp p;
	std::vector<void *> vec;

	D("allocating objects of size: ");
	D("8 bytes");
	vec.push_back(new object<8>);
	D("16 bytes");
	vec.push_back(new object<16>);
	D("32 bytes");
	vec.push_back(new object<32>);
	D("64 bytes");
	vec.push_back(new object<64>);
	D("128 bytes");
	vec.push_back(new object<128>);
	D("256 bytes");
	vec.push_back(new object<256>);
	D("512 bytes");
	vec.push_back(new object<512>);

	for(auto & it: vec)
	{
		delete it;			
	}
}

BOOST_AUTO_TEST_CASE(std_vector_test)
{
	sp p;
	const size_t items_to_test = 1000 * 1000;

	D("vector: ");
	{
		std::vector<int> vec;
	
		milli_timer t("\ttlslib allocation");
		for(size_t index = 0; index < items_to_test; ++index)
		{
			vec.push_back(index);
		}
	}

	{
		std::vector<int, malloc_allocator<int>> vec;
	
		milli_timer t("\tmalloc allocation");
		for(size_t index = 0; index < items_to_test; ++index)
		{
			vec.push_back(index);
		}
	}
}

BOOST_AUTO_TEST_CASE(std_list_test)
{
	sp p;
	const size_t items_to_test = 1000 * 200;

	D("list:");
	{
		std::list<int> vec;
		
		milli_timer t("\ttlslib allocation");
		for(size_t index = 0; index < items_to_test; ++index)
		{
			vec.push_back(index);
		}
	}

	{
		std::list<int, malloc_allocator<int>> vec;
	
		milli_timer t("\tmalloc allocation");
		for(size_t index = 0; index < items_to_test; ++index)
		{
			vec.push_back(index);
		}
	}
}

BOOST_AUTO_TEST_CASE(std_vector_test_parallel)
{
	sp p;
	const size_t items_to_test = 1000 * 1000;
	const size_t thread_count = 4;

	D("vector in " << thread_count << " threads: ");


	auto tls_alloc = [=]()
	{
		std::vector<int> tls_vec; 

		for(size_t index = 0; index < items_to_test; ++index)
		{
			tls_vec.push_back(index);
		}
	};
	
	auto mal_alloc = [=]()
	{
		std::vector<int, malloc_allocator<int>> mal_vec;

		for(size_t index = 0; index < items_to_test; ++index)
		{
			mal_vec.push_back(index);
		}
	};

	starter(tls_alloc, thread_count, "\ttls allocation");
	starter(mal_alloc, thread_count, "\tmalloc allocation");
}

BOOST_AUTO_TEST_CASE(std_list_test_parallel)
{
	sp p;
	const size_t items_to_test = 1000 * 50;
	const size_t thread_count = 4;

	D("list in " << thread_count << " threads: ");


	auto tls_alloc = [=]()
	{
		std::list<int> tls_vec; 

		for(size_t index = 0; index < items_to_test; ++index)
		{
			tls_vec.push_back(index);
		}
	};
	
	auto mal_alloc = [=]()
	{
		std::list<int, malloc_allocator<int>> mal_vec;

		for(size_t index = 0; index < items_to_test; ++index)
		{
			mal_vec.push_back(index);
		}
	};

	starter(tls_alloc, thread_count, "\ttls allocation");
	starter(mal_alloc, thread_count, "\tmalloc allocation");
}
