#include <iostream>
#include <random>
#include <algorithm>

#include <boost/math/tools/polynomial.hpp>

#include "FFT.hpp"

#define BOOST_TEST_MODULE FFT_TEST
#ifdef _WIN32
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif

BOOST_AUTO_TEST_CASE(random_test)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 10000);
	auto generator = [&]() { return dis(gen); };

	const size_t vector_length = 100000;
	std::vector<int> vec1, vec2;
	vec1.reserve(vector_length);
	std::generate_n(std::back_inserter(vec1), vector_length, generator);
	std::generate_n(std::back_inserter(vec2), vector_length, generator);

	std::vector<int> res1, res2;
	multiply(vec1, vec2, res1);
	multiply_simple(vec1, vec2, res2);

	for (size_t index = 0; index < res1.size(); ++index)
	{
		BOOST_CHECK(res1[index] == res2[index]);
	}
}

BOOST_AUTO_TEST_CASE(small_test)
{
	using boost::math::tools::polynomial;
	const size_t test_size = 100;
	
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 1000);
	auto generator = [&]() { return dis(gen); };

	std::vector<long> vec1, vec2;
	vec1.reserve(test_size);
	std::generate_n(std::back_inserter(vec1), test_size, generator);
	std::generate_n(std::back_inserter(vec2), test_size, generator);

	polynomial<long> p1(vec1.data(), vec1.size() - 1);
	polynomial<long> p2(vec2.data(), vec2.size() - 1);

	auto p3 = p1 * p2;

	std::vector<long> res1;
	multiply(vec1, vec2, res1);

	for(size_t index = 0; index <= p3.degree(); ++index)
	{
		BOOST_CHECK(res1[index] == p3[index]);
	}
	std::cout << std::endl;
}
