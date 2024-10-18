#include "resolver.h"
#include "rule.h"
#include <algorithm>
#include <iostream>
#include <iterator>

struct DisjunctionComparator {
  bool operator()(const Rule::ptr &left, const Rule::ptr &right) const {
    int leftSize = left->getOperands().size();
    int rightSize = right->getOperands().size();
    return leftSize <= rightSize;
  }
};

static bool isInversePair(Rule::ptr left, Rule::ptr right) {
  return Rule::createInverse(left)->toNormalForm()->toString() ==
         right->toNormalForm()->toString();
}

static Rule::ptr tryResolve(Rule::ptr disjunction1, Rule::ptr disjunction2) {
  auto disAtoms1 = disjunction1->getOperands();
  auto disAtoms2 = disjunction2->getOperands();

  // std::cout << "try resolve for " << disjunction1->toString() << " and "
  //           << disjunction2->toString() << std::endl;
  //
  // std::cout << "disAtoms1: ";
  // for (const auto &rule : disAtoms1)
  //   std::cout << rule->toString() << ", ";
  // std::cout << "\ndisAtoms2: ";
  // for (const auto &rule : disAtoms2)
  //   std::cout << rule->toString() << ", ";
  // std::cout << std::endl;

  for (int i = 0; i < disAtoms1.size(); ++i) {
    for (int j = 0; j < disAtoms2.size(); ++j) {
      if (isInversePair(disAtoms1[i], disAtoms2[j])) {
        // std::cout << "inverse pair found at i = " << i << ", j = " << j
        //           << std::endl;
        disAtoms1.erase(disAtoms1.begin() + i);
        disAtoms2.erase(disAtoms2.begin() + j);

        std::copy(disAtoms2.begin(), disAtoms2.end(),
                  std::back_inserter(disAtoms1));
        return std::make_shared<Rule>(Rule::Type::disjunction, disAtoms1);
      }
    }
  }

  return nullptr;
}

bool Resolver::Implies(std::list<Rule::ptr> source, Rule::ptr target) {
  if (source.size() == 0)
    return false;
  auto sourceAnd = Rule::createConjunction(source.begin(), source.end());
  return Implies(sourceAnd, target);
}

bool Resolver::Implies(Rule::ptr source, Rule::ptr target) {
  // выделить список элементарных дизъюнктов
  m_axiomSet = source->toNormalForm()->getDisjunctionsList();
  m_referenceSet =
      Rule::createInverse(target)->toNormalForm()->getDisjunctionsList();

  // отсортировать списки в порядке возрастания кол-ва атомов
  m_axiomSet.sort(DisjunctionComparator{});
  m_referenceSet.sort(DisjunctionComparator{});

  PrintState();

  // реализуем стратегию с опорным множеством
  while (!m_referenceSet.empty()) {
    // берем один дизъюнкт из опорного множества и ищем ему пару среди
    // дизъюнктов опорного множества и, если не нашли - ищем в аксиомах.
    auto ref = m_referenceSet.front();
    m_referenceSet.pop_front();

    std::list<Rule::ptr>::iterator pair = m_referenceSet.begin();
    while (pair != m_referenceSet.end()) {
      if (auto res = tryResolve(ref, *pair)) {
        if (res->getOperands().size() == 0)
          return true;
        m_referenceSet.erase(pair);
        m_referenceSet.push_front(res);
        std::cout << "resolved with refset\n";
        PrintState();
        break;
      }
      pair++;
    }
    if (pair == m_referenceSet.end()) {
      // не нашли пару среди опорных дизъюнктов - ищем среди аксиом
      pair = m_axiomSet.begin();
      while (pair != m_axiomSet.end()) {
        if (auto res = tryResolve(ref, *pair)) {
          if (res->getOperands().size() == 0)
            return true;
          m_axiomSet.erase(pair);
          m_referenceSet.push_front(res);
          std::cout << "resolved with axioms\n";
          PrintState();
          break;
        }
        pair++;
      }
    }
    // если ничего не нашли - значит резолюция для данного дизъюнкта невозможна
    // - просто отбрасываем его
  };

  // обошли все возможные дизъюнкты в опорном множестве и не получили пустой
  // дизъюнкт - исходное утверждение не доказано
  return false;
}

void Resolver::PrintState() {
  std::cout << "axioms: ";
  bool first = true;
  for (const auto &rule : m_axiomSet) {
    if (!first)
      std::cout << ", ";
    first = false;
    std::cout << rule->toString();
  }
  std::cout << "\nrefset: ";
  first = true;
  for (const auto &rule : m_referenceSet) {
    if (!first)
      std::cout << ", ";
    first = false;
    std::cout << rule->toString();
  }
  std::cout << std::endl;
}
