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
  int srcNode = -1;
  int dstNode = -1;
  const char *svgFilename = NULL;
  const char *databaseFilename = defaultDatabaseFilename;
  bool verboseImport = false;
  bool onlyDFS = false;
  bool onlyBFS = false;

  constexpr auto helpMessage =
      R"( [-i database.txt] [-v] [-o output.svg] [--only-dfs] [--only-bfs] <srcNode> <dstNode>

    -i database.txt load database from given file (default: database.txt)
    -v              print verbose information about imported database
    -o output.svg   export database as SVG image graph with graphviz
    --only-dfs      run only depth first search
    --only-bfs      run only breadth first search
    <srcNode>       number of start node (default: random)
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
    } else if (!strcmp(argv[i], "--only-dfs")) {
      onlyDFS = true;
    } else if (!strcmp(argv[i], "--only-bfs")) {
      onlyBFS = true;
    } else if (i == argc - 2) {
      srcNode = atoi(argv[i]);
    } else if (i == argc - 1) {
      dstNode = atoi(argv[i]);
    } else {
      std::cout << argv[0] << helpMessage;
      return 0;
    }
  }

  if (onlyDFS && onlyBFS) {
    std::cerr << "--only-dfs and --only-bfs options are mutually exclusive"
              << std::endl;
    return -1;
  }
  if (!onlyDFS && !onlyBFS) {
    onlyDFS = true;
    onlyBFS = true;
  }

  std::shared_ptr<Dictionary> database;
  try {
    database = std::make_shared<Dictionary>(databaseFilename);
  } catch (std::exception e) {
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

  if (srcNode == -1) {
    srcNode = rand() % database->FactsCount();
  }
  if (dstNode == -1) {
    dstNode = rand() % database->FactsCount();
  }

  std::cout << "start node: " << srcNode << ", end node: " << dstNode
            << std::endl;

  if (onlyDFS) {
    GraphSearch gs(database->Rules(), srcNode, dstNode);
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
  }

  if (onlyBFS) {
    GraphSearch gs(database->Rules(), srcNode, dstNode);
    auto rulesBFS = gs.DoBreadthFirstSearch();

    std::cout << "BFS: ";
    for (auto rule : rulesBFS)
      std::cout << rule << " -> ";
    std::cout << std::endl << "-----------------------------" << std::endl;

    if (svgFilename) {
      GraphViz()
          .DefineGraph(database->Rules())
          .DrawPath(rulesBFS)
          .Export(svgFilename);
    }
  }

  return 0;
}
