#include <memory>
#include <limits>

template<class T>
class malloc_allocator : public std::allocator<T>
{
public:
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;

	template<class U>
	struct rebind
	{
		typedef malloc_allocator<U> other;
	};

	template<class U>
	malloc_allocator(const malloc_allocator<U> &) {}

	malloc_allocator() {}

	pointer address(reference value) const 
	{
		return &value;
	}
	const_pointer address(const_reference value) const 
	{
		return &value;
	}

	size_type max_size() const throw() 
	{
		return std::numeric_limits<std::size_t>::max() / sizeof(T);
	}

	// allocate but don't initialize num elements of type T
	pointer allocate(size_type num, const void* = 0) 
	{

		pointer ret = (pointer) malloc(num * sizeof(T));
		return ret;
	}

	// initialize elements of allocated storage p with value value
	void construct(pointer p, const T& value) 
	{
		new((void*)p)T(value);
	}

	void destroy(pointer p) 
	{
		p->~T();
	}

	// deallocate storage p of deleted elements
	void deallocate(pointer p, size_type num) 
	{
		free((void*)p);
	}
};

template <class T1, class T2>
bool operator== (const malloc_allocator<T1>&, const malloc_allocator<T2>&) throw() 
{
	return true;
}

template <class T1, class T2>
bool operator!= (const malloc_allocator<T1>&, const malloc_allocator<T2>&) throw()
{
	return false;
}