CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -I.
TARGET := server

.PHONY: all clean run

all: $(TARGET)

$(TARGET): Main.cc
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET) 8080

clean:
	rm -f $(TARGET)
