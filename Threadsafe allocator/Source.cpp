#include <thread>
#include <iostream>
#include <vector>
#include <boost/thread.hpp>
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

struct point
{
	int x, y;
};

int main()
{
	tls::main_block_pool pool;
	tls::local_block_pool local_pool(pool);

	
	for (size_t index = 0; index < 1000; ++index)
	{
		void * new_block = local_pool.new_block(sizeof(point));
		point * pp = new(new_block) point();
		pp->x = index;
		pp->y = index;
	}

	return 0;
}