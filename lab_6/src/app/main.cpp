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
    auto res = Solver(database).solveForward(*target);
    if (res)
      std::cout << res->toString() << std::endl;
    else
      std::cout << "not proven" << std::endl;
  }
  return 0;
}
