#include <iostream>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>

#define TEST

#ifdef TEST
#define BOOST_TEST_MODULE LUP_TEST
#ifdef _WIN32
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif
#endif
#include "LUP_decomposition.hpp"

using namespace boost::numeric::ublas;

BOOST_AUTO_TEST_CASE(lup_test)
{
	const size_t matrix_size = 4;
	matrix<double> A(matrix_size, matrix_size), B((identity_matrix<double>(matrix_size)));

	

	Matrix<double> m = Matrix<double>::random(matrix_size, matrix_size);

	for (size_t index = 0; index < 4; index++)
	{
		for (size_t jndex = 0; jndex < 4; jndex++)
		{
			A(index, jndex) = m.at(index, jndex);
		}
	}

	permutation_matrix<> pm(4); 

	lu_factorize<matrix<double>, permutation_matrix<> >(A, pm);

	std::vector<size_t> perm;
	auto t = m.lup_decomposition(perm, 4);

	for (size_t index = 0; index < 4; index++)
	{
		for (size_t jndex = 0; jndex < 4; jndex++)
		{
			BOOST_REQUIRE(abs(t.at(index, jndex) - A(index, jndex)) < 0.00001);
		}
	}
}

BOOST_AUTO_TEST_CASE(slu_test)
{
	const size_t matrix_size = 100;
	auto m = Matrix<long>::random(matrix_size, matrix_size);
	matrix<double> m1(matrix_size, matrix_size);

	for (size_t row = 0; row < matrix_size; ++row)
	{
		for (size_t column = 0; column < matrix_size; ++column)
		{
			m1(row, column) = m.at(row, column);
		}
	}

	auto b = Matrix<long>::random(1, matrix_size);
	//std::vector<long> b = { 2, 0, 1, 12 };
	auto x = m.solve_slu(b);

	matrix<double> b1(1, matrix_size);
	matrix<double> x1(matrix_size, 1);
	for (size_t index = 0; index < matrix_size; ++index)
	{
		x1(index, 0) = x[index];
		b1(0, index) = b.at(0, index);
	}
	
	auto mx = prod(m1, x1);
	for (size_t index = 0; index < matrix_size; ++index)
	{
		BOOST_CHECK(abs(b.at(0, index) -mx(index, 0)) < 0.00001);
	}
}
