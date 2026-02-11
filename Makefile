CXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -pthread -Isrc
LIBS=-lsqlite3
OUT=server


SRCS=src/main.cpp src/db.cpp src/geo.cpp src/timeutil.cpp
HDRS=src/crow_all.h src/db.h src/geo.h src/timeutil.h


all: $(OUT)

$(OUT): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)
