# ===================================================
# Makefile for BloomFusionDB
# ===================================================
#
# Build targets:
#   make         -> builds the interactive menu app (./db)
#   make bench   -> builds the benchmark (./benchmark)
#   make all     -> builds both
#   make clean   -> removes all built files
#
# Usage:
#   ./db            -> run the interactive menu
#   ./benchmark     -> run performance comparison

CXX      = g++
CXXFLAGS = -std=c++11 -O2 -Wall -I include

# Source files shared by both targets
SHARED_SRCS = src/BloomFilter.cpp src/FusionTree.cpp src/Database.cpp

# Object files (one per .cpp)
SHARED_OBJS = BloomFilter.o FusionTree.o Database.o

.PHONY: all clean

all: db benchmark interview

# ---- Interactive menu app ----
db: $(SHARED_OBJS) main.o
	$(CXX) $(CXXFLAGS) -o db $(SHARED_OBJS) main.o

main.o: src/main.cpp include/Database.h
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o main.o

# ---- Benchmark app ----
bench: benchmark

benchmark: $(SHARED_OBJS) benchmark.o
	$(CXX) $(CXXFLAGS) -o benchmark $(SHARED_OBJS) benchmark.o

benchmark.o: src/benchmark.cpp include/Database.h
	$(CXX) $(CXXFLAGS) -c src/benchmark.cpp -o benchmark.o

# ---- Shared object rules ----
BloomFilter.o: src/BloomFilter.cpp include/BloomFilter.h
	$(CXX) $(CXXFLAGS) -c src/BloomFilter.cpp -o BloomFilter.o

FusionTree.o: src/FusionTree.cpp include/FusionTree.h
	$(CXX) $(CXXFLAGS) -c src/FusionTree.cpp -o FusionTree.o

Database.o: src/Database.cpp include/Database.h include/BloomFilter.h include/FusionTree.h
	$(CXX) $(CXXFLAGS) -c src/Database.cpp -o Database.o

# ---- Cleanup ----
clean:
	rm -f *.o db benchmark interview

# ---- Interview winning test ----
interview: $(SHARED_OBJS) interview_test.o
	$(CXX) $(CXXFLAGS) -o interview $(SHARED_OBJS) interview_test.o

interview_test.o: src/interview_test.cpp include/Database.h
	$(CXX) $(CXXFLAGS) -c src/interview_test.cpp -o interview_test.o