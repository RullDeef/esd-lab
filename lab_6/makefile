CXX      := g++
CXXFLAGS := --std=c++20 -Isrc/core
SRCS     := $(wildcard src/**/*.cpp)

app: $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^
