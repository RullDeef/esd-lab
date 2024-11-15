#include "prolog_solver.h"
#include <iostream>

PrologSolver::PrologSolver(const Database &database) : Solver(database) {}

void PrologSolver::solveBackwardThreaded(Atom target, Channel<Subst> &output) {
  auto dbStart = m_database.getRules().begin();

  m_resolventStack.clear();
  m_resolventStack.push_front({{target, dbStart}});
  m_backtrackStack.clear();
  m_backtrackStack.push_front({});
  m_varsStack.clear();
  m_targetCounter = 1;

  while (!m_resolventStack.empty()) {
    showState();
    std::list<AtomEx> &currTargets = m_resolventStack.front();
    // если все подцели доказаны - запускаем возврат
    while (currTargets.empty()) {
      if (m_backtrackStack.front().empty()) {
        std::cout << "empty backtrack stack!\n";
        return;
      } else {
        auto &btrackAtoms = m_backtrackStack.front();
        m_resolventStack.pop_front();
        m_varsStack.pop_front();
        m_resolventStack.push_front(btrackAtoms);
        m_backtrackStack.pop_front();
      }
    }
    // доказываем текущую подцель
    AtomEx *currTarget = &currTargets.front();
    std::cout << "curr target: " << currTarget->atom.toString() << " "
              << std::distance(m_database.getRules().begin(), currTarget->label)
              << "/" << m_database.rulesCount() << std::endl;
    // начинаем поиск правила, которое может доказать текущую подцель
    bool currTargetResolved = false;
    while (!currTargetResolved &&
           currTarget->label != m_database.getRules().end()) {
      std::cout << "checking rule " << currTarget->label->toString()
                << std::endl;
      Subst subst = Subst(); // varsStack.empty() ? Subst() : varsStack.front();
      if (unify(currTarget->atom, currTarget->label->getOutput(), subst)) {
        std::cout << "unified!\n";
        // переложить атом из верхушки стека резольвенты в стек отката
        m_backtrackStack.push_front({});
        m_backtrackStack.front().push_front(*currTarget);
        currTargets.pop_front();
        currTarget = &m_backtrackStack.front().front();
        m_varsStack.push_front(subst);
        m_targetCounter += currTarget->label->getInputs().size() - 1;
        // добавить входы правила в стек резольвенты, если есть
        if (currTarget->label->isFact()) {
          if (m_targetCounter == 0) {
            Subst filtered;
            for (auto &arg : target.getArguments()) {
              if (arg->getValue() != "_" && arg->isVariable())
                filtered.insert(arg->getValue(),
                                m_varsStack.front().apply(arg));
            }
            if (!output.put(filtered)) {
              return;
            }
          }
        } else {
          m_resolventStack.push_front({});
          for (auto input : currTarget->label->getInputs()) {
            m_resolventStack.front().push_front(
                {subst.apply(input), m_database.getRules().begin()});
          }
        }
        currTargetResolved = true;
        std::cout << "resolved! " << subst.toString() << std::endl;
      }
      currTarget->label++;
    }
    if (!currTargetResolved) {
      // откат
      if (m_backtrackStack.empty()) {
        // доказательство текущей цели невозможно - перебрали все возможные
        // варианты
        return;
      } else {
        m_targetCounter++;
        auto &btrackAtoms = m_backtrackStack.front();
        for (auto &atom : m_resolventStack.front()) {
          m_targetCounter--;
        }
        m_resolventStack.pop_front();
        m_varsStack.pop_front();
        m_resolventStack.front() = btrackAtoms;
        m_backtrackStack.pop_front();
      }
    }
  }
}

void PrologSolver::showState() const {
  std::cout << "=============================================\n";
  std::cout << "stacks:\n";
  std::cout << "  resolvents:\n";
  for (auto &res : m_resolventStack) {
    std::cout << "    {";
    for (auto &atomEx : res)
      std::cout << atomEx.atom.toString() << "["
                << std::distance(m_database.getRules().begin(), atomEx.label)
                << "/" << m_database.rulesCount() << "], ";
    std::cout << "}\n";
  }
  std::cout << "  backtrack:\n";
  for (auto &atomsEx : m_backtrackStack) {
    std::cout << "    {";
    for (auto &atomEx : atomsEx)
      std::cout << atomEx.atom.toString() << "["
                << std::distance(m_database.getRules().begin(), atomEx.label)
                << "/" << m_database.rulesCount() << "], ";
    std::cout << "}" << std::endl;
  }
  std::cout << "  substs:\n";
  for (auto &subst : m_varsStack)
    std::cout << "    " << subst.toString() << std::endl;
  std::cout << "=============================================\n";
}
