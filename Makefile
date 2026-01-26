CXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -pthread -Isrc
OUT=server

all: $(OUT)

$(OUT): src/main.cpp src/crow_all.h
	$(CXX) $(CXXFLAGS) src/main.cpp -o $(OUT)

clean:
	rm -f $(OUT)
