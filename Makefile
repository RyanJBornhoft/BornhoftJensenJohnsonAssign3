CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -g -fdiagnostics-color=always

# Defaults (override on command line: make run1 SRC=foo.asm, make run2 INT=foo.int)
SRC ?= test.asm
INT ?= test.int

COMMON_OBJS := SymbolTable.o LiteralTable.o OpcodeTable.o

all: Pass1 Pass2

Pass1: Pass1.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

Pass2: Pass2.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f Pass1 Pass2 *.o *.obj *.txt *.int

# Convenience run targets
run1: Pass1
	./Pass1 $(SRC)

run2: Pass2
	./Pass2 $(INT)

.PHONY: all clean run1 run2
