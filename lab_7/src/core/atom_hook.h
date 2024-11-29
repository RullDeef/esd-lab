#pragma once

#include "channel.h"
#include "solver.h"
#include "subst.h"
#include "variable.h"
#include <exception>
#include <functional>
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

class IntOp3Hook : public AtomHook {
public:
  using op = std::function<int(int, int)>;
  IntOp3Hook(const char *name, op opRes, op opInvFirst, op opInvSecond)
      : AtomHook(name), m_op(opRes), m_opInvFirst(opInvFirst),
        m_opInvSecond(opInvSecond) {}

protected:
  virtual void proveThreaded(std::vector<Variable::ptr> args, Subst subst,
                             std::shared_ptr<Channel<Subst>> output) override {
    // there must be exactly 3 arguments
    if (args.size() != 3)
      return;
    // two of three arguments must be bound to a constant
    int numBound = int(args[0]->isConst()) + int(args[1]->isConst()) +
                   int(args[2]->isConst());
    if (numBound < 2)
      return;
    try {
      if (numBound == 3) {
        if (verifyResult(args[0], args[1], args[2]))
          output->put(std::move(subst));
      } else if (args[2]->isVariable()) // if result is not bound - compute it
        computeResult(args[0], args[1], args[2], subst);
      else if (args[0]->isVariable()) // if first arg is not bound - compute it
        computeFirstInput(args[0], args[1], args[2], subst);
      else // if second args is not bound - compute it
        computeSecondInput(args[0], args[1], args[2], subst);
      output->put(std::move(subst));
    } catch (std::exception &err) {
      std::cerr << getName() << ": " << err.what() << std::endl;
    }
  }

private:
  bool verifyResult(Variable::ptr first, Variable::ptr second,
                    Variable::ptr res) {
    int firstVal = std::stoi(first->getValue());
    int secondVal = std::stoi(second->getValue());
    int resVal = std::stoi(res->getValue());
    return m_op(firstVal, secondVal) == resVal;
  }

  void computeResult(Variable::ptr first, Variable::ptr second,
                     Variable::ptr res, Subst &subst) {
    int firstVal = std::stoi(first->getValue());
    int secondVal = std::stoi(second->getValue());
    auto resVal = std::to_string(m_op(firstVal, secondVal));
    subst.insert(res->getValue(), Variable::createConst(resVal));
  }

  void computeFirstInput(Variable::ptr first, Variable::ptr second,
                         Variable::ptr res, Subst &subst) {
    int resVal = std::stoi(res->getValue());
    int secondVal = std::stoi(second->getValue());
    auto firstVal = std::to_string(m_opInvFirst(secondVal, resVal));
    subst.insert(first->getValue(), Variable::createConst(firstVal));
  }

  void computeSecondInput(Variable::ptr first, Variable::ptr second,
                          Variable::ptr res, Subst &subst) {
    int resVal = std::stoi(res->getValue());
    int firstVal = std::stoi(first->getValue());
    auto secondVal = std::to_string(m_opInvSecond(firstVal, resVal));
    subst.insert(second->getValue(), Variable::createConst(secondVal));
  }

  op m_op;
  op m_opInvFirst;
  op m_opInvSecond;
};

class IntAddHook : public IntOp3Hook {
public:
  IntAddHook()
      : IntOp3Hook(
            "add", [](int first, int second) { return first + second; },
            [](int second, int res) { return res - second; },
            [](int first, int res) { return res - first; }) {}
};

class IntMulHook : public IntOp3Hook {
public:
  IntMulHook()
      : IntOp3Hook(
            "mul", [](int first, int second) { return first * second; },
            [](int second, int res) { return res / second; },
            [](int first, int res) { return res / first; }) {}
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

// in_range(var, start, end) <=> (start <= var < end)
// or act as a generator for free variable var
class InRangeHook : public AtomHook {
public:
  InRangeHook() : AtomHook("in_range") {}

protected:
  virtual void proveThreaded(std::vector<Variable::ptr> args, Subst subst,
                             std::shared_ptr<Channel<Subst>> output) override {
    // there must be exactly 3 arguments
    if (args.size() != 3)
      return;
    // second and third arguments must be bound to constant value
    if (!args[1]->isConst() || !args[2]->isConst())
      return;
    // second and third arguments must be numerical values
    try {
      int start = std::stoi(args[1]->getValue());
      int end = std::stoi(args[2]->getValue());
      doRange(start, end, std::move(args[0]), std::move(subst),
              std::move(output));
    } catch (...) {
    }
  }

private:
  void doRange(int start, int end, Variable::ptr var, Subst subst,
               std::shared_ptr<Channel<Subst>> output) {
    if (var->isConst()) {
      try {
        int value = std::stoi(var->getValue());
        if (start <= value && value < end)
          output->put(std::move(subst));
      } catch (...) {
      }
    } else if (var->isVariable()) {
      for (int value = start; value < end; ++value) {
        Subst newSubst = subst;
        newSubst.insert(var->getValue(),
                        Variable::createConst(std::to_string(value)));
        if (!output->put(std::move(newSubst)))
          break;
      }
    }
  }
};
