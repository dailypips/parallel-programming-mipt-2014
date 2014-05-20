#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

template<class T>
class Matrix
{
private:
	std::vector<T> data;
	size_t rows;
	size_t columns;

	friend class Matrix;

	class lu_adapter
	{
	private:
		 Matrix<double>  m;
	public:
		lu_adapter(const Matrix<double> & m_) :
			m(m_)
		{
		}

		double getU(size_t row, size_t column)
		{
			if (row > column)
			{
				throw std::invalid_argument("this is upper-triangle matrix");
			}
			return m.at(row, column);
		}

		double getL(size_t row, size_t column)
		{
			if (row < column)
			{
				throw std::invalid_argument("this is lower-triangle matrix");
			}

			if (row == column)
			{
				return 1;
			}

			return m.at(row, column);
		}
	};

public:
	Matrix(size_t rows, size_t columns)
	{
		this->rows = rows;
		this->columns = columns;

		data.resize(rows * columns);
	}

	Matrix(size_t rows, size_t columns, const std::initializer_list<T> & list) :
		rows(rows),
		columns(columns),
		data(list)
	{
	}

	template<class _T>
	Matrix(const Matrix<_T> & other)
	{
		rows = other.rows;
		columns = other.columns;
		data.resize(rows * columns);

		for (size_t row = 0; row < rows; row++)
		{
			for (size_t column = 0; column < columns; column++)
			{
				at(row, column) = other.at(row, column);
			}
		}
	}

	T & at(size_t row, size_t column)
	{
		return data[row * columns + column];
	}

	const T & at(size_t row, size_t column) const
	{
		return data[row * columns + column];
	}

	void swap_rows(size_t row1, size_t row2)
	{
		for (size_t column = 0; column < columns; column++)
		{
			std::swap(at(row1, column), at(row2, column));
		}
	}

	Matrix<double> lup_decomposition(std::vector<size_t> & perms, size_t thread_count) const
	{
		perms.resize(rows);
		std::iota(perms.begin(), perms.end(), 0);

		Matrix<double> result(*this);

		for (size_t phase = 0; phase < rows; ++phase)
		{
			double p = 0;
			size_t max_index = 0;
			for (size_t row = phase; row < rows; row++)
			{
				double elem = abs(result.at(row, phase));
				if (elem > p)
				{
					p = elem;
					max_index = row;
				}
			}

			if (p == 0)
			{
				throw std::logic_error("matrix is singular");
			}

			std::swap(perms[phase], perms[max_index]);
			result.swap_rows(phase, max_index);

			auto row_changer = [&](size_t row_index)
			{
				for (int row = rows - 1 - row_index; row > (int)phase; row -= thread_count)
				{
					result.at(row, phase) /= result.at(phase, phase);

					for (size_t index = phase + 1; index < rows; index++)
					{
						result.at(row, index) -= result.at(row, phase) * result.at(phase, index);
					}
				}
			};
			
			std::vector<boost::unique_future<void>> futures;
			
			for (size_t index = 0; index < thread_count - 1; ++index)
			{
				futures.push_back(boost::async(std::bind(row_changer, index)));
			}

			row_changer(thread_count - 1);

			for (auto & it : futures)
			{
				it.get();
			}
		}

		return result;
	}

	std::vector<double> solve_slu(const Matrix<T> & b)
	{
		std::vector<size_t> perms;
		auto lu = lup_decomposition(perms, 4);

		lu_adapter ad(lu);
		std::vector<double> y(rows);
		std::vector<double> x(rows);

		for (size_t row = 0; row < rows; ++row)
		{
			y[row] = b.at(0, perms[row]);
			for (size_t column = 0; column < row; ++column)
			{
				y[row] -= ad.getL(row, column) * y[column];
			}
		}

		for (int row = rows - 1; row >= 0; --row)
		{
			x[row] = y[row];
			for (size_t column = row + 1; column < rows; ++column)
			{
				x[row] -= ad.getU(row, column) * x[column];
			}
			x[row] /= ad.getU(row, row);
		}

		return x;
	}

	void print()
	{
		for (size_t row = 0; row < rows; row++)
		{
			for (size_t column = 0; column < columns; column++)
			{
				std::cout << at(row, column) << "\t";
			}
			std::cout << std::endl;
		}
	}

	static Matrix<T> random(size_t rows, size_t columns)
	{
		Matrix<T> result(rows, columns);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, 10000);

		std::generate(result.data.begin(), result.data.end(), [&]() { return dis(gen); });
		return result;
	}
};