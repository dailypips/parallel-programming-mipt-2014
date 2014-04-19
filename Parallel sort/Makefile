CC = clang++
CFLAGS = -std=c++11
LIBRARY = /usr/include/boost
INCLUDE = -I$(LIBRARY)
THREADLIB = -pthread
BOOSTLIBS = -lboost_thread -lboost_system
OUT = ThPool
all: $(OUT) clean

$(OUT):
	$(CC) $(CFLAGS) Future.hpp  Sorter.hpp  Source.cpp  ThreadPool.hpp  ThreadsafePriorityQueue.hpp $(THREADLIB) $(INCLUDE) $(BOOSTLIBS)

clean:
	rm *.gch
