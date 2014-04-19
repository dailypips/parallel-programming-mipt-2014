#include <iostream>
#include <ostream>

#include "ParallelScan.hpp"

class Permutation
{
private:
	std::vector<int> indexes;

public:
	Permutation(const std::initializer_list<int> & list) :
		indexes(list)
	{
	}

	Permutation()
	{
	}

	Permutation operator*(const Permutation & other) const
	{
		Permutation result;
		for (auto & it : indexes)
		{
			result.indexes.push_back(other.indexes[it - 1]);
		}
		return result;
	}

private:
	friend std::ostream& operator<<(std::ostream &, const Permutation &);
};

std::ostream& operator<<(std::ostream & stream, const Permutation & perm)
{
	for (auto & it : perm.indexes)
	{
		stream << it << " ";
	}
	stream << std::endl;
	return stream;
}

int main()
{
	std::vector<Permutation> vec;
	// this permitation generates a group of order 12, so a^13 = a. Let's check it
	auto il = { 2, 3, 4, 1, 6, 7, 5 };
	for (size_t index = 0; index < 13; ++index)
	{
	vec.push_back(il);
	}

	auto mul = [](const Permutation & a, const Permutation & b)
	{
	return a * b;
	};

	parallel_scan(vec.begin(), vec.end(), mul);

	for (auto & it : vec)
	{
		std::cout << it;
	}

	return 0;
}