#include "Sorter.hpp"

#define BOOST_TEST_MODULE SorterTest
#include <boost\test\included\unit_test.hpp>

BOOST_AUTO_TEST_CASE(normal_work_test)
{
	std::vector<int> toSort = { { 5, 4, 3, 2, 1 } };
	std::vector<int> result = { { 1, 2, 3, 4, 5 } };

	my::sort(toSort.begin(), toSort.end());

	BOOST_CHECK(std::equal(toSort.begin(), toSort.end(), result.begin()));
}

BOOST_AUTO_TEST_CASE(empty_test)
{
	std::vector<int> vector;
	BOOST_CHECK_NO_THROW(my::sort(vector.begin(), vector.end()));
}

BOOST_AUTO_TEST_CASE(big_test)
{
	std::vector<int> vector;
	const int elemCount = 1000;
	for (size_t index = elemCount; index > 0; --index)
	{
		vector.push_back(index);
	}

	my::sort(vector.begin(), vector.end());

	for (size_t index = 0; index < elemCount; ++index)
	{
		BOOST_CHECK_EQUAL(vector[index], index + 1);
	}
}

BOOST_AUTO_TEST_CASE(equal_test)
{
	std::vector<int> vector = { { 7, 7, 7, 7, 7, 7 } };
	my::sort(vector.begin(), vector.end());
}

BOOST_AUTO_TEST_CASE(already_sorted_test)
{
	std::vector<int> vector = { { 1, 2, 3, 4, 5, 6 } };
	my::sort(vector.begin(), vector.end());
}

BOOST_AUTO_TEST_CASE(custom_comparer_test)
{
	using Pair = std::pair<int, int>;
	std::vector<Pair> vector;
	const int size = 5;
	for (size_t index = 0; index < size; ++index)
	{
		vector.emplace_back(size - index, size - index);
	}

	my::sort(vector.begin(), vector.end(), 
		[](const Pair & pair1, const Pair & pair2)
	{
		return pair1.first < pair2.first || pair1.second < pair2.second;
	});

	for (size_t index = 0; index < size; ++index)
	{
		BOOST_CHECK_EQUAL(vector[index].first, index + 1);
	}
}