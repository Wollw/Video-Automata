TARGET=video-automata
CXX = g++
CXXFLAGS = -Wall -Werror -std=c++11
LIBS=opencv

all:
	g++ $(CXXFLAGS) src/main.cpp -o bin/$(TARGET) `pkg-config $(LIBS) --libs --cflags`

debug:
	g++ $(CXXFLAGS) src/main.cpp -o bin/$(TARGET) `pkg-config $(LIBS) --libs --cflags` -g

clean:
	if [ -e bin/$(TARGET) ]; then rm bin/$(TARGET); fi
