CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -I include

SHARED_SRCS = src/BloomFilter.cpp src/FusionTree.cpp src/Database.cpp
SHARED_OBJS = BloomFilter.o FusionTree.o Database.o

.PHONY: all clean

all: db benchmark test

db: $(SHARED_OBJS) main.o
	$(CXX) $(CXXFLAGS) -o db $(SHARED_OBJS) main.o

main.o: src/main.cpp include/Database.h
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o main.o

bench: benchmark

benchmark: $(SHARED_OBJS) benchmark.o
	$(CXX) $(CXXFLAGS) -o benchmark $(SHARED_OBJS) benchmark.o

benchmark.o: src/benchmark.cpp include/Database.h
	$(CXX) $(CXXFLAGS) -c src/benchmark.cpp -o benchmark.o

BloomFilter.o: src/BloomFilter.cpp include/BloomFilter.h
	$(CXX) $(CXXFLAGS) -c src/BloomFilter.cpp -o BloomFilter.o

FusionTree.o: src/FusionTree.cpp include/FusionTree.h
	$(CXX) $(CXXFLAGS) -c src/FusionTree.cpp -o FusionTree.o

Database.o: src/Database.cpp include/Database.h include/BloomFilter.h include/FusionTree.h
	$(CXX) $(CXXFLAGS) -c src/Database.cpp -o Database.o

clean:
	rm -f *.o db benchmark test

test: $(SHARED_OBJS) test.o
	$(CXX) $(CXXFLAGS) -o test $(SHARED_OBJS) test.o

test.o: src/test.cpp include/Database.h
	$(CXX) $(CXXFLAGS) -c src/test.cpp -o test.o