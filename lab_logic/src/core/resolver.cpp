#include "resolver.h"
#include "rule.h"
#include "substitution.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <optional>

struct DisjunctionComparator {
  bool operator()(const Rule::ptr &left, const Rule::ptr &right) const {
    int leftSize = left->getOperands().size();
    int rightSize = right->getOperands().size();
    return leftSize <= rightSize;
  }

  bool operator()(const std::pair<Rule::ptr, Substitution> &left,
                  const std::pair<Rule::ptr, Substitution> &right) const {
    return this->operator()(left.first, right.first);
  }
};

static bool startsUpper(const std::string &value) {
  return !value.empty() && std::isupper(value[0]);
}

static std::optional<Substitution> unifyTerms(Rule::ptr left, Rule::ptr right,
                                              bool topLevel = true) {
  bool left_predicate = !left->getOperands().empty();
  bool left_var = !left_predicate && startsUpper(left->getValue()) && !topLevel;
  bool left_atom = !left_predicate && !left_var;
  bool right_predicate = !right->getOperands().empty();
  bool right_var =
      !right_predicate && startsUpper(right->getValue()) && !topLevel;
  bool right_atom = !right_predicate && !right_var;

  if (left_atom && right_atom) {
    return left->getValue() == right->getValue()
               ? std::make_optional<Substitution>()
               : std::nullopt;
  } else if ((left_atom || left_predicate) && right_var) {
    Substitution subst;
    subst.add(right->getValue(), left);
    return std::move(subst);
  } else if ((right_atom || right_predicate) && left_var) {
    Substitution subst;
    subst.add(left->getValue(), right);
    return std::move(subst);
  } else if (left_predicate && right_predicate) {
    if (left->getValue() != right->getValue())
      return std::nullopt;
    auto leftOperands = left->getOperands();
    auto rightOperands = right->getOperands();
    if (leftOperands.size() != rightOperands.size())
      return std::nullopt;
    Substitution subst;
    for (int i = 0; i < leftOperands.size(); ++i) {
      auto part = unifyTerms(leftOperands[i], rightOperands[i], false);
      if (!part)
        return std::nullopt;
      subst += *part;
    }
    return std::move(subst);
  } else if (left_var && right_var) {
    Substitution subst;
    subst.add(left->getValue(), right);
    return std::move(subst);
  }
  return std::nullopt;
}

static std::optional<Substitution> unifyInversePair(Rule::ptr left,
                                                    Rule::ptr right) {
  if (left->getType() == Rule::Type::inverse &&
      right->getType() == Rule::Type::atom)
    return unifyInversePair(std::move(right), std::move(left));
  if (left->getType() != Rule::Type::atom ||
      right->getType() != Rule::Type::inverse)
    return std::nullopt;
  return unifyTerms(left, right->getOperands()[0]);
}

static std::optional<std::pair<Rule::ptr, Substitution>>
tryResolve(const std::pair<Rule::ptr, Substitution> &disjunction1,
           const std::pair<Rule::ptr, Substitution> &disjunction2) {
  auto disAtoms1 = disjunction1.first->getOperands();
  auto disAtoms2 = disjunction2.first->getOperands();

  std::optional<Substitution> subst;
  for (int i = 0; i < disAtoms1.size(); ++i) {
    for (int j = 0; j < disAtoms2.size(); ++j) {
      subst = unifyInversePair(disAtoms1[i], disAtoms2[j]);
      if (subst) {
        disAtoms1.erase(disAtoms1.begin() + i);
        disAtoms2.erase(disAtoms2.begin() + j);
        std::copy(disAtoms2.begin(), disAtoms2.end(),
                  std::back_inserter(disAtoms1));
        break;
      }
    }
    if (subst)
      break;
  }
  if (!subst)
    return std::nullopt;
  for (auto &atom : disAtoms1)
    atom = subst->applyTo(std::move(atom));
  *subst += disjunction1.second;
  *subst += disjunction2.second;
  auto rule = std::make_shared<Rule>(Rule::Type::disjunction, disAtoms1);
  return std::make_pair(rule, *subst);
}

bool Resolver::Implies(std::list<Rule::ptr> source, Rule::ptr target) {
  Rule::ptr sourceAnd;
  if (source.size() > 0)
    sourceAnd = Rule::createConjunction(source.begin(), source.end());
  return Implies(sourceAnd, target);
}

bool Resolver::Implies(Rule::ptr source, Rule::ptr target) {
  if (target->toString() == "1")
    return true;
  if (target->toString() == "0")
    return false;

  // выделить список элементарных дизъюнктов
  m_axiomSet =
      source
          ? disjunctionsTransform(source->toNormalForm()->getDisjunctionsList())
          : std::list<std::pair<Rule::ptr, Substitution>>{};
  m_referenceSet = disjunctionsTransform(
      Rule::createInverse(target)->toNormalForm()->getDisjunctionsList());

  // отсортировать списки в порядке возрастания кол-ва атомов
  m_axiomSet.sort(DisjunctionComparator{});
  m_referenceSet.sort(DisjunctionComparator{});

  PrintState();

  // реализуем стратегию с опорным множеством
  while (!m_referenceSet.empty()) { // && !m_axiomSet.empty()) {
    // берем один дизъюнкт из опорного множества и ищем ему пару среди
    // дизъюнктов опорного множества и, если не нашли - ищем в аксиомах.
    auto ref = m_referenceSet.front();
    m_referenceSet.pop_front();

    std::list<std::pair<Rule::ptr, Substitution>>::iterator pair =
        m_referenceSet.begin();
    while (pair != m_referenceSet.end()) {
      if (auto res = tryResolve(ref, *pair)) {
        if (res->first->getOperands().size() == 0) {
          std::cout << "found empty disjunction with refset: "
                    << ref.first->toString() << " and "
                    << pair->first->toString()
                    << " (subst: " << pair->second.toString() << ")"
                    << std::endl;
          return true;
        }
        std::cout << "resolved with refset: " << ref.first->toString()
                  << " and " << pair->first->toString()
                  << " (subst: " << pair->second.toString() << ")" << std::endl;
        m_referenceSet.erase(pair);
        m_referenceSet.push_front(*res);
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
          if (res->first->getOperands().size() == 0) {
            std::cout << "found empty disjunction with axiom: "
                      << ref.first->toString() << " and "
                      << pair->first->toString()
                      << " (subst: " << res->second.toString() << ")"
                      << std::endl;
            return true;
          }
          std::cout << "resolved with axioms: " << ref.first->toString()
                    << " and " << pair->first->toString()
                    << " (subst: " << res->second.toString() << ")"
                    << std::endl;
          m_axiomSet.erase(pair);
          m_referenceSet.push_front(*res);
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

std::list<std::pair<Rule::ptr, Substitution>>
Resolver::disjunctionsTransform(std::list<Rule::ptr> disjunctions) {
  std::list<std::pair<Rule::ptr, Substitution>> res;
  for (auto &disjunction : disjunctions)
    res.push_back(
        std::make_pair<Rule::ptr, Substitution>(std::move(disjunction), {}));
  return res;
}

void Resolver::PrintState() {
  std::cout << "axioms: ";
  bool first = true;
  for (const auto &rule : m_axiomSet) {
    if (!first)
      std::cout << ", ";
    first = false;
    std::cout << rule.first->toString();
    if (!rule.second.empty())
      std::cout << rule.second.toString();
  }
  std::cout << "\nrefset: ";
  first = true;
  for (const auto &rule : m_referenceSet) {
    if (!first)
      std::cout << ", ";
    first = false;
    std::cout << rule.first->toString();
    if (!rule.second.empty())
      std::cout << rule.second.toString();
  }
  std::cout << std::endl;
}
