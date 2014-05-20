CC = clang++
CFLAGS = -std=c++11
LIBRARY = /usr/include/boost
INCLUDE = -I$(LIBRARY)
THREADLIB = -pthread
BOOSTLIBS = -lboost_thread -lboost_system
OUT = ThPool
FILES = Source.cpp
all: $(OUT)

$(OUT):
	$(CC) $(CFLAGS) $(FILES) $(THREADLIB) $(INCLUDE) $(BOOSTLIBS)

d:
	$(CC) $(CFLAGS) -g $(FILES) $(THREADLIB) $(INCLUDE) $(BOOSTLIBS)

opt: 
	$(CC) $(CFLAGS) -O4 $(FILES) $(THREADLIB) $(INCLUDE) $(BOOSTLIBS)
