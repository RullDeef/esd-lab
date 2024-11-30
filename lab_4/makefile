CC := g++

SRCS := $(wildcard src/**/*.cpp) $(wildcard src/**/**/*.cpp)

app: $(SRCS)
	g++ -Isrc/core -o $@ $^
