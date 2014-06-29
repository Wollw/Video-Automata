TARGET=video-automaton
CXX = g++
LIBS=opencv libgflags

all:
	g++ src/main.cpp -o bin/$(TARGET) `pkg-config $(LIBS) --libs --cflags`

debug:
	g++ src/main.cpp -o bin/$(TARGET) `pkg-config $(LIBS) --libs --cflags` -g

clean:
	if [ -e bin/$(TARGET) ]; then rm bin/$(TARGET); fi
