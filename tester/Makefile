CXX = g++
CXXFLAGS = -std=c++17 -I ~/local/include -pthread
LDFLAGS = -L ~/local/lib -lgtest -lgtest_main -no-pie

TARGET = test 

SRC = main.cpp 

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean
