#include "atom_hook.h"
#include "database.h"
#include "mgraph_solver.h"
#include "parser.h"
#include <iostream>
#include <memory>
#include <optional>
#include <utility>

// pre-defined atom hooks
std::map<std::string, std::shared_ptr<AtomHook>> buildPredefinedHooks() {
  return {
      {"write", std::make_shared<WriteHook>()},
      {"leq", std::make_shared<LeqHook>()},
      {"in_range", std::make_shared<InRangeHook>()},
  };
}

std::pair<std::optional<Atom>, bool> inputTarget(bool &run,
                                                 Database &database) {
  std::cout << "?- ";
  std::string line;
  if (!std::getline(std::cin, line)) {
    run = false;
    return std::make_pair(std::nullopt, false);
  }
  if (!line.empty() && line[0] == '+') {
    // insert mode - add new rule to database
    try {
      auto rule = RuleParser().ParseRule(line.c_str() + 1);
      database.addRule(rule);
    } catch (std::exception &err) {
      std::cerr << "parse error: " << err.what() << std::endl;
      return std::make_pair(std::nullopt, false);
    }
    return inputTarget(run, database);
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
  auto database = std::make_shared<Database>();
  if (argc == 2)
    database = std::make_shared<Database>(argv[1]);
  else if (argc != 1) {
    std::cout << "usage: " << argv[0] << " [database.txt]" << std::endl;
    return -1;
  }

  // start repl
  bool run = true;
  while (run) {
    auto [target, forward] = inputTarget(run, *database);
    if (!target)
      continue;
    auto solver =
        std::make_shared<MGraphSolver>(database, buildPredefinedHooks());
    if (forward)
      solver->solveForward(*target);
    else
      solver->solveBackward(*target);
    std::string line;
    do {
      auto subst = solver->next();
      if (!subst) {
        std::cout << "end" << std::endl;
        break;
      } else {
        std::cout << subst->toString() << std::endl;
        std::cout << "next?- ";
        std::getline(std::cin, line);
      }
    } while (!line.empty());
    solver->done();
  }
  return 0;
}
