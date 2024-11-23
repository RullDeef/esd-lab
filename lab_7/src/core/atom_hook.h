#pragma once

#include "solver.h"
#include "subst.h"
#include "variable.h"
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

class AtomHook {
public:
  AtomHook(const char *name) : m_name(name) {}
  virtual ~AtomHook() = default;

  const char *getName() const { return m_name; }

  // returns worker thread and output solutions channel.
  // Can define arbitrary logic for proving this atom hook
  TaskChanPair<Subst> prove(std::vector<Variable::ptr> args, Subst subst) {
    auto output = std::make_shared<Channel<Subst>>();
    std::jthread worker(
        [this, args = std::move(args), subst = std::move(subst), output]() {
          proveThreaded(std::move(args), std::move(subst), output);
          output->close();
        });
    return std::make_pair(std::move(worker), output);
  }

protected:
  virtual void proveThreaded(std::vector<Variable::ptr> args, Subst subst,
                             std::shared_ptr<Channel<Subst>> output) = 0;

private:
  const char *m_name;
};

// pre-defined hooks
class WriteHook : public AtomHook {
public:
  WriteHook() : AtomHook("write") {}

protected:
  virtual void proveThreaded(std::vector<Variable::ptr> args, Subst subst,
                             std::shared_ptr<Channel<Subst>> output) override {
    bool first = true;
    std::stringstream s;
    for (auto &arg : args) {
      if (!first)
        s << " ";
      first = false;
      s << arg->toString();
    }
    s << std::endl;
    std::cout << s.str();
    output->put(subst);
  }
};

class LeqHook : public AtomHook {
public:
  LeqHook() : AtomHook("leq") {}

protected:
  virtual void proveThreaded(std::vector<Variable::ptr> args, Subst subst,
                             std::shared_ptr<Channel<Subst>> output) override {
    // there must be exactly 2 arguments
    if (args.size() != 2)
      return;
    // both arguments must be bound to a constant value (we do not support
    // constraint programming for now)
    if (!args[0]->isConst() || !args[1]->isConst())
      return;
    // both constants must be numerical values
    try {
      int left = std::stoi(args[0]->getValue());
      int right = std::stoi(args[1]->getValue());
      std::cout << "comparing " << left << " and " << right << std::endl;
      if (left <= right)
        output->put(subst);
    } catch (...) {
    }
  }
};
