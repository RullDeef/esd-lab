#include "database.h"
#include "parser.h"
#include "solver.h"
#include <iostream>
#include <optional>

std::optional<Atom> inputTarget(bool &run) {
  std::cout << "?- ";
  std::string line;
  if (!std::getline(std::cin, line)) {
    run = false;
    return std::nullopt;
  }
  try {
    return RuleParser().ParseFact(line.c_str());
  } catch (std::exception &err) {
    std::cerr << "parse error: " << err.what() << std::endl;
    return std::nullopt;
  }
}

int main(int argc, char **argv) {
  Database database;
  if (argc == 2)
    database = Database(argv[1]);
  else if (argc != 1) {
    std::cout << "usage: " << argv[0] << " [database.txt]" << std::endl;
    return -1;
  }

  // start repl
  bool run = true;
  while (run) {
    auto target = inputTarget(run);
    if (!target)
      continue;
    Solver solver(database);
    solver.solveForward(*target);
    std::string line;
    do {
      auto subst = solver.next();
      if (!subst) {
        std::cout << "end" << std::endl;
        break;
      } else {
        std::cout << subst->toString() << std::endl;
        std::cout << "next?- ";
        std::getline(std::cin, line);
      }
    } while (!line.empty());
  }
  return 0;
}
