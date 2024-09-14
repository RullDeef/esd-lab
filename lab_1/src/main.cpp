#include <iostream>
#include <memory>
#include "dict.h"
#include "graph_search.h"
#include "rule_printer.h"
#include "graph_viz.h"

int main() {
    auto dict = std::make_shared<Dictionary>("kb.txt");
    std::cout << "initial knowledge base:" << std::endl;
    RulePrinter printer(dict);
    for (auto rule : dict->Rules())
        printer.PrintRule(rule);
    
    int srcNode = 7;
    int dstNode = 4;

    GraphSearch gs(dict->Rules(), srcNode, dstNode);
    auto rules = gs.DoDepthFirstSearch();

    std::cout << "DFS: ";
    for (auto rule : rules)
        std::cout << rule << " -> ";
    std::cout << std::endl;

    gs = GraphSearch(dict->Rules(), srcNode, dstNode);
    rules = gs.DoBreadthFirstSearch();

    std::cout << "BFS: ";
    for (auto rule : rules)
        std::cout << rule << " -> ";
    std::cout << std::endl;

    GraphViz()
        .DefineGraph(dict->Rules())
        .DrawPath(rules)
        .Export("kb.svg");
    
    return 0;
}
