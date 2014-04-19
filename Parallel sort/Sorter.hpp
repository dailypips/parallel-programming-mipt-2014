#include <algorithm>
#include <iterator>

#include "ThreadPool.hpp"

namespace my {

	template<class ForwardIt, class Cmp = std::less<typename std::iterator_traits<ForwardIt>::value_type>>
	void sort(ForwardIt begin, ForwardIt end, Cmp cmp = Cmp())
	{
		using T = typename std::iterator_traits<ForwardIt>::value_type;

		SimpleThreadPool pool;

		std::function<void(ForwardIt, ForwardIt)> sorter;
		sorter = [&](ForwardIt begin, ForwardIt end)
		{
			while (std::distance(begin, end) > 2)
			{				
				size_t size = std::distance(begin, end);
				auto pivotIter = begin;
				std::advance(pivotIter, size / 2);
				T const & pivot = *pivotIter;

				ForwardIt middle1 = std::partition(begin, end,
					[&](const T & item)
				{
					return cmp(item, pivot);
				});

				ForwardIt middle2 = std::partition(middle1, end,
					[&](const T & item)
				{
					return !cmp(pivot, item);
				});


				pool.runAsync<std::function<void()>>(std::bind(sorter, middle2, end));
				end = middle1;
			}
		};

		sorter(begin, end);
	}
}
