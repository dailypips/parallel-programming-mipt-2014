CC = g++
CFLAGS = -std=c++11
LIBRARY = /usr/include/boost
INCLUDE = -I$(LIBRARY)
THREADLIB = -pthread
BOOSTLIBS = -lboost_thread -lboost_system
OUT = ThPool
all: $(OUT) clean

$(OUT):
	$(CC) $(CFLAGS) Source.cpp Logger.h $(THREADLIB) $(INCLUDE) $(BOOSTLIBS)

clean:
	rm *.gch
