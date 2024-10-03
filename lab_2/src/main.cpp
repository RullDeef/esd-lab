#include "dict.h"
#include "graph_search.h"
#include "graph_viz.h"
#include "rule_printer.h"
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>

constexpr auto defaultDatabaseFilename = "database.txt";

int main(int argc, char **argv) {
  std::vector<int> srcNodes;
  int dstNode = -1;
  const char *svgFilename = NULL;
  const char *databaseFilename = defaultDatabaseFilename;
  bool verboseImport = false;

  constexpr auto helpMessage =
      R"( [-i database.txt] [-v] [-o output.svg] <srcNodes> <dstNode>

    -i database.txt load database from given file (default: database.txt)
    -v              print verbose information about imported database
    -o output.svg   export database as SVG image graph with graphviz
    <srcNodes>      numbers of start nodes (default: 1 random)
    <dstNode>       number of destination node (default: random)
)";

  if (argc < 3) {
    std::cout << argv[0] << helpMessage;
    return 0;
  }

  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-i")) {
      databaseFilename = argv[++i];
    } else if (!strcmp(argv[i], "-v")) {
      verboseImport = true;
    } else if (!strcmp(argv[i], "-o")) {
      svgFilename = argv[++i];
    } else if (i < argc - 1) {
      srcNodes.push_back(atoi(argv[i]));
    } else if (i == argc - 1) {
      dstNode = atoi(argv[i]);
    } else {
      std::cout << argv[0] << helpMessage;
      return 0;
    }
  }

  std::shared_ptr<Dictionary> database;
  try {
    database = std::make_shared<Dictionary>(databaseFilename);
  } catch (const std::exception &e) {
    std::cerr << "failed to load database \"" << databaseFilename << "\""
              << std::endl;
    return 1;
  }
  if (verboseImport) {
    std::cout << "initial knowledge base:" << std::endl;
    RulePrinter printer(database);
    for (auto rule : database->Rules())
      printer.PrintRule(rule);
    std::cout << "-----------------------" << std::endl;
  }

  if (srcNodes.empty()) {
    srcNodes.push_back(rand() % database->FactsCount());
  }
  if (dstNode == -1) {
    dstNode = rand() % database->FactsCount();
  }

  std::cout << "start nodes:";
  for (auto node : srcNodes)
    std::cout << ' ' << node;
  std::cout << ", end node: " << dstNode << std::endl;

  GraphSearch gs(database->Rules(), srcNodes, dstNode);
  auto rulesDFS = gs.DoDepthFirstSearch();

  std::cout << "DFS: ";
  for (auto rule : rulesDFS)
    std::cout << rule << " -> ";
  std::cout << std::endl << "-----------------------------" << std::endl;

  if (svgFilename) {
    GraphViz()
        .DefineGraph(database->Rules())
        .DrawPath(rulesDFS)
        .Export(svgFilename);
  }

  return 0;
}
