CXX := g++
CXXFLAGS := --std=c++17 -Wall -Werror -pedantic

.PHONY: all clean

all: app
	./app -i kb.txt 1 5

clean:
	rm -rf obj

app: obj/dict.o obj/graph_search.o obj/graph_viz.o obj/main.o obj/rule_printer.o
	$(CXX) $(CXXFLAGS) -o $@ $^

obj/dict.o obj/graph_search.o obj/graph_viz.o obj/main.o obj/rule_printer.o: obj/%.o: src/%.cpp
	mkdir -p $(dir $@) && $(CXX) $(CXXFLAGS) -o $@ -c $^

