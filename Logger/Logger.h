#include <ostream>
#include <boost\thread.hpp>
#include <mutex>
#include <chrono>
#include <ctime>
#include <atomic>

class Logger
{
private:
	struct LogEntry
	{
		std::string message;
		std::time_t dateTime;

		void store(const std::string & message)
		{
			using namespace std::chrono;
			this->message = message;
			dateTime = system_clock::to_time_t(system_clock::now());
		}

		void printTo(std::ostream & out)
		{
			out << std::put_time(std::localtime(&dateTime), "%c") << ": " << message << std::endl;
		}
	};

	const size_t BUFFER_SIZE = 100;

	std::ostream & out;
	LogEntry * buffer;
	size_t bufferIndex;
	boost::detail::spinlock lock;
	std::mutex flushLock;
	std::atomic_int writersCount;

public:
	Logger(std::ostream & out) :
		out(out),
		bufferIndex(0)
	{
		buffer = new LogEntry[BUFFER_SIZE];
		lock.unlock();
		writersCount.store(0);
	}

	Logger& operator<<(const std::string & message)
	{
		while (true)
		{
			std::unique_lock<boost::detail::spinlock> lk(lock);
			LogEntry * bufferPtr = buffer;
			int index = bufferIndex++;
			lk.unlock();

			if (index < BUFFER_SIZE)
			{
				++writersCount;
				bufferPtr[index].store(message);
				--writersCount;
				return *this;
			}

			LogEntry * newBuffer = new LogEntry[BUFFER_SIZE];
			lk.lock();
			if (bufferPtr == buffer)
			{
				buffer = newBuffer;
				bufferIndex = 1;
				lk.unlock();
			}
			else
			{
				delete[] newBuffer;
				continue;
			}

			newBuffer[0].store(message);
			flush(bufferPtr, BUFFER_SIZE);
			delete[] bufferPtr;
			return *this;
		}
	}

	template<class T>
	Logger& operator<<(T value)
	{
		std::string s = std::to_string(value);
		return *this << s;
	}

	~Logger()
	{
		flush(buffer, bufferIndex);
		delete[] buffer;
	}

private:
	void flush(LogEntry * buffer, size_t size)
	{
		while (writersCount.load() != 0)
			;

		std::lock_guard<std::mutex> lock(flushLock);
		for (size_t index = 0; index < size; ++index)
		{
			buffer[index].printTo(out);
		}
	}
};