#include <vector>
#include <complex>
#include <cmath>
#include <thread>
#include <functional>
#include <boost/thread/future.hpp>
#include <boost/thread.hpp>

typedef std::complex<long double> base;
const double PI = 4 * atan(1);

inline size_t fill_to_2power(size_t n)
{
	return (size_t)(exp2((size_t)(ceil(log2(n)))));
}

unsigned int rev_bits(unsigned int num, unsigned int lg_n)
{
	int res = 0;
	for (int i = 0; i < lg_n; ++i)
	{
		if (num & (1 << i))
			res |= 1 << (lg_n - 1 - i);
	}
	return res;
}

template<class T>
std::vector<base> bit_reverse_copy(const std::vector<T> & array)
{
	std::vector<base> result(array.size());
	size_t lg_n = (size_t) log2(array.size());
	for (size_t index = 0; index < array.size(); ++index)
	{
		result[rev_bits(index, lg_n)] = array[index];
	}
	return result;
}

template<class Func>
void parallel_process(size_t begin, size_t end, Func func, size_t step)
{
	size_t length = end - begin;
	size_t items_to_work = length / step;
	size_t thread_count = std::thread::hardware_concurrency();
	thread_count = thread_count == 0 ? 2 : thread_count;

	auto applier = [&](size_t _begin, size_t _end)
	{
		for (; _begin != _end; _begin += step)
		{
			func(_begin);
		}
	};

	if (items_to_work < 4)
	{
		applier(begin, end);
	}
	else
	{
		std::vector<boost::unique_future<void>> futures;
		size_t block_size = (items_to_work + 0.5 * thread_count) / thread_count;

		for (size_t index = 0; index < thread_count - 1; ++index)
		{
			size_t _begin = begin + index * block_size * step;
			size_t _end = _begin + block_size * step;
			auto binded_applier = std::bind(applier, _begin, _end);
			futures.push_back(boost::async(binded_applier));
		}

		size_t _begin = begin + (thread_count - 1) * block_size * step;
		size_t _end = length;
		if (_begin < _end)
		{
			applier(_begin, _end);
		}

		for (auto & fut : futures)
		{
			fut.get();
		}
	}
}

template<class T>
void fft(std::vector<T> & array, bool invert)
{
	if (array.empty())
	{
		return;
	}

	size_t power2 = fill_to_2power(array.size());
	array.resize(power2);

	size_t array_length = array.size();

	auto rev_array = bit_reverse_copy(array);

	auto applier = [&](size_t len, base wlen, size_t k)
	{
		base w(1);
		for (size_t j = 0; j < len / 2; ++j)
		{
			base u = rev_array[k + j], v = rev_array[k + j + len / 2] * w;
			rev_array[k + j] = u + v;
			rev_array[k + j + len / 2] = u - v;
			w *= wlen;
		}
	};

	for (size_t len = 2; len <= array_length; len <<= 1)
	{
		double angle = 2 * PI / len * (invert ? -1 : 1);
		base wlen(cos(angle), sin(angle));

		auto binded_applier = std::bind(applier, len, wlen, std::placeholders::_1);
		parallel_process(0, array_length, binded_applier, len);
	}
	if (invert)
	{
		auto applier = [&](size_t index)
		{
			rev_array[index] /= array_length;
		};

		parallel_process(0, array_length, applier, 1);
	}

	std::swap(array, rev_array);
}

template<class T>
void fft_simple(std::vector<T> & a, bool invert) 
{
	int n = (int)a.size();
	if (n == 1)  return;

	std::vector<base> a0(n / 2), a1(n / 2);
	for (int i = 0, j = 0; i<n; i += 2, ++j) {
		a0[j] = a[i];
		a1[j] = a[i + 1];
	}
	fft(a0, invert);
	fft(a1, invert);

	double ang = 2 * PI / n * (invert ? -1 : 1);
	base w(1), wn(cos(ang), sin(ang));
	for (int i = 0; i<n / 2; ++i) {
		a[i] = a0[i] + w * a1[i];
		a[i + n / 2] = a0[i] - w * a1[i];
		if (invert)
			a[i] /= 2, a[i + n / 2] /= 2;
		w *= wn;
	}
}

template<class T>
void multiply(const std::vector<T> & a, const std::vector<T> & b, std::vector<T> & res) 
{
	std::vector<base> fa(a.begin(), a.end()), fb(b.begin(), b.end());
	size_t n = 1;
	while (n < std::max(a.size(), b.size()))  n <<= 1;
	n <<= 1;
	fa.resize(n), fb.resize(n);

	fft(fa, false), fft(fb, false);
	for (size_t i = 0; i<n; ++i)
		fa[i] *= fb[i];
	fft(fa, true);

	res.resize(n);
	for (size_t i = 0; i<n; ++i)
		res[i] = T(fa[i].real() + 0.5);
}

void multiply_simple(const std::vector<int> & a, const std::vector<int> & b, std::vector<int> & res)
{
	std::vector<base> fa(a.begin(), a.end()), fb(b.begin(), b.end());
	size_t n = 1;
	while (n < std::max(a.size(), b.size()))  n <<= 1;
	n <<= 1;
	fa.resize(n), fb.resize(n);

	fft_simple(fa, false), fft_simple(fb, false);
	for (size_t i = 0; i<n; ++i)
		fa[i] *= fb[i];
	fft_simple(fa, true);

	res.resize(n);
	for (size_t i = 0; i<n; ++i)
		res[i] = int(fa[i].real() + 0.5);
}
