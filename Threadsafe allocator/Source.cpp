#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>

#include "TLSLib.hpp"


void print_sizes()
{
	std::cout << sizeof(tls::super_block_t<8>) << std::endl;
	std::cout << sizeof(tls::super_block_t<16>) << std::endl;
	std::cout << sizeof(tls::super_block_t<32>) << std::endl;
	std::cout << sizeof(tls::super_block_t<64>) << std::endl;
	std::cout << sizeof(tls::super_block_t<128>) << std::endl;
	std::cout << sizeof(tls::super_block_t<256>) << std::endl;
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
	int x, y;
};



void * operator new(size_t n) throw(std::bad_alloc)
{	
	static tls::main_pool main_pool;
	static tls::local_pool local_pool(main_pool);
	return local_pool.new_mem(n);
}

void operator delete(void * p) throw()
{
	//return local_pool.free_mem(p);
}

int main()
{
	std::vector<void *> mems;

	{
		timer t("acquiring memory");
		for (size_t index = 0; index < 10000; ++index)
		{
			//void * mem = local_pool.new_mem(sizeof(point));
			point * pp = new point();
			mems.push_back(pp);
		}
	}

	{
		timer t("freeing memory");
		for (size_t index = 0; index < 10000; ++index)
		{
			//local_pool.free_mem(mems[index]);
			delete mems[index];
		}
	}
	return 0;
}