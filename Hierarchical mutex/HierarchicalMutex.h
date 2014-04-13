#include <mutex>
#include <map>
#include <stdexcept>
#include <boost/thread.hpp>

template<class T>
class ThreadLocal
{
	std::map<boost::thread::id, size_t> indexes;
	std::vector<T> values;
	std::mutex mutex;

public:
	T& get()
	{
		std::lock_guard<std::mutex> lk(mutex);
		auto tid = boost::this_thread::get_id();
		if (indexes.find(tid) == indexes.end())
		{
			indexes[tid] = values.size();
			values.emplace_back();
		}
		return values[indexes[tid]];
	}
};

class HierarchicalMutex
{
private:
	std::mutex internal_mutex;
	unsigned long hierarchy_value;
	std::list<unsigned long>::iterator this_mutex_priority;
	static ThreadLocal<std::list<unsigned long>> this_thread_hierarchy;

	void check_hierarchy()
	{
		if (this_thread_hierarchy.get().empty())
		{
			return;
		}

		if (this_thread_hierarchy.get().back() <= hierarchy_value)
		{
			throw std::logic_error("mutex hierarchy violated");
		}
	}

	void update_hierarchy_value()
	{
		auto & hier = this_thread_hierarchy.get();
		this_mutex_priority = hier.insert(hier.end(), hierarchy_value);
	}

public:
	explicit HierarchicalMutex(unsigned long value) :
		hierarchy_value(value),
		this_mutex_priority(this_thread_hierarchy.get().end())
	{
	}

	void lock()
	{
		check_hierarchy();
		internal_mutex.lock();
		update_hierarchy_value();
	}

	void unlock()
	{
		this_thread_hierarchy.get().erase(this_mutex_priority);
		internal_mutex.unlock();
	}

	bool try_lock()
	{
		check_hierarchy();
		if (!internal_mutex.try_lock())
		{
			return false;
		}

		update_hierarchy_value();
		return true;
	}
};

ThreadLocal<std::list<unsigned long>> HierarchicalMutex::this_thread_hierarchy;
