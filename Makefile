CC = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
SRC = Pass1.cpp SymbolTable.cpp LiteralTable.cpp OpcodeTable.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = pass1

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
