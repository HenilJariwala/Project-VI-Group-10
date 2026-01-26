CXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -pthread -Isrc
LIBS=-lsqlite3
OUT=server

all: $(OUT)

$(OUT): src/main.cpp src/db.cpp src/crow_all.h src/db.h
	$(CXX) $(CXXFLAGS) src/main.cpp src/db.cpp -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)
