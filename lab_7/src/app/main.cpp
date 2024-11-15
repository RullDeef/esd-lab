#include "database.h"
#include "parser.h"
#include "solver.h"
#include <iostream>
#include <optional>
#include <utility>

std::pair<std::optional<Atom>, bool> inputTarget(bool &run) {
  std::cout << "?- ";
  std::string line;
  if (!std::getline(std::cin, line)) {
    run = false;
    return std::make_pair(std::nullopt, false);
  }
  const auto forward = line.empty() || line[0] != '!';
  try {
    auto rule =
        RuleParser().ParseRule(line.c_str() + static_cast<int>(!forward));
    if (!rule.isFact()) {
      std::cerr << "fact expected, got rule" << std::endl;
      return std::make_pair(std::nullopt, forward);
    }
    return std::make_pair(rule.getOutput(), forward);
  } catch (std::exception &err) {
    std::cerr << "parse error: " << err.what() << std::endl;
    return std::make_pair(std::nullopt, false);
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
    auto [target, forward] = inputTarget(run);
    if (!target)
      continue;
    Solver solver(database);
    if (forward)
      solver.solveForward(*target);
    else
      solver.solveBackward(*target);
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
