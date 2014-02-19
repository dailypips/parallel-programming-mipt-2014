#pragma once
#include <mutex>
#include <condition_variable>
#include <memory>

template<class T>
class DataContainer
{
private:
	bool isReady;
	T data;
	std::shared_ptr<std::exception> exception;

	std::mutex mutex;
	std::condition_variable condition;
	
public:
	DataContainer() :
		isReady(false)
	{
	}

	T get()
	{
		std::unique_lock<std::mutex> lock(mutex);
		while(!isReady)
		{
			condition.wait(lock, [this]() -> bool { return isReady; });
		}
		if (exception)
		{
			throw *exception;
		}
		return data;
	}

	void set(const T & data)
	{
		this->data = data;
		isReady = true;
		condition.notify_all();
	}

	void setException(const std::exception & e)
	{
		exception = std::make_shared<std::exception>(e);		
		isReady = true;
		condition.notify_all();
	}
};

template<class T>
class Future
{
private:
	std::shared_ptr<DataContainer<T>> ptr;
public:
	Future() :
		ptr(std::make_shared<DataContainer<T>>())
	{
	}

	T get()
	{
		return ptr->get();
	}

	void set(const T & data)
	{
		ptr->set(data);
	}

	void setException(const std::exception & e)
	{
		ptr->setException(e);
	}
};

// partial specializations for void 
template<>
class DataContainer<void>
{
private:
	bool isReady;
	std::shared_ptr<std::exception> exception;

	std::mutex mutex;
	std::condition_variable condition;
	
public:
	DataContainer() :
		isReady(false)
	{
	}

	void get()
	{
		std::unique_lock<std::mutex> lock(mutex);
		while(!isReady)
		{
			condition.wait(lock, [this]() -> bool { return isReady; });
		}
		if (exception)
		{
			throw *exception;
		}
	}

	void set()
	{
		isReady = true;
		condition.notify_all();
	}

	void setException(const std::exception & e)
	{
		exception = std::make_shared<std::exception>(e);
		set();
	}
};

template<>
class Future<void>
{
private:
	std::shared_ptr<DataContainer<void>> ptr;
public:
	Future() :
		ptr(std::make_shared<DataContainer<void>>())
	{
	}

	void get()
	{
		ptr->get();
	}

	void set()
	{
		ptr->set();
	}

	void setException(const std::exception & e)
	{
		ptr->setException(e);
	}
};