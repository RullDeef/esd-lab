#include "subst.h"
#include "solver.h"
#include <algorithm>

template <typename T>
static bool set_overlaps(const std::set<T> &set1, const std::set<T> &set2) {
  std::set<T> ref;
  std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                        std::inserter(ref, ref.end()), std::less());
  return !ref.empty();
}

std::optional<Subst> Subst::operator+(const Subst &other) const {
  if (m_pairs.empty() && m_links.empty())
    return other;
  if (other.m_pairs.empty() && other.m_links.empty())
    return *this;
  // start building combined substitution
  Subst newSubst = *this;
  for (const auto &[var, value] : other.m_pairs) {
    if (m_pairs.count(var) != 0 &&
        m_pairs.at(var)->toString() != value->toString()) {
      // попытка унификации двух термов с возможной генерацией новой
      // подстановки
      Subst auxSubst;
      if (!Solver::unify(m_pairs.at(var), value,
                         auxSubst)) // унификация невозможна - конфликт
        return std::nullopt;
      newSubst.insert(var, auxSubst.apply(value));
    } else if (!newSubst.insert(var, value))
      return std::nullopt;
  }
  for (const auto &otherRing : other.m_links) {
    bool intersected = false;
    for (auto &ring : newSubst.m_links) {
      if (set_overlaps(ring, otherRing)) {
        ring.insert(otherRing.begin(), otherRing.end());
        if (!ringValid(ring))
          return std::nullopt;
        intersected = true;
        break;
      }
    }
    if (!intersected) {
      if (!ringValid(otherRing))
        return std::nullopt;
      newSubst.m_links.push_back(otherRing);
    }
  }
  // solve recursive references
  for (auto &[var, value] : newSubst.m_pairs)
    newSubst.solveRecursion(value);
  return newSubst;
}

bool Subst::insert(const std::string &var, Variable::ptr value) {
  if (var == "_" || value->getValue() == "_")
    return true;
  if (m_pairs.count(var) > 0) {
    return m_pairs[var]->toString() == value->toString();
  }
  m_pairs[var] = value;
  // to avoid check assigment does not conflicts with linked variables
  // propagate value to all linked variables
  for (auto &ring : m_links)
    if (ring.count(var) > 0)
      for (const auto &linkedVar : ring)
        m_pairs[linkedVar] = value;
  return true;
}

bool Subst::link(const std::string &var1, const std::string &var2) {
  if (var1 == "_" || var2 == "_")
    return true;
  if (m_pairs.count(var1) > 0 && m_pairs.count(var2) > 0) {
    if (m_pairs.at(var1) != m_pairs.at(var2))
      return false;
  }
  for (auto ring : m_links) {
    if (ring.count(var1) > 0) {
      ring.insert(var2);
      if (m_pairs.count(var1) > 0)
        m_pairs[var2] = m_pairs[var1];
      return true;
    } else if (ring.count(var2) > 0) {
      ring.insert(var1);
      if (m_pairs.count(var2) > 0)
        m_pairs[var1] = m_pairs[var2];
      return true;
    }
  }
  m_links.push_back({var1, var2});
  if (m_pairs.count(var1) > 0)
    m_pairs[var2] = m_pairs[var1];
  else if (m_pairs.count(var2) > 0)
    m_pairs[var1] = m_pairs[var2];
  return true;
}

Variable::ptr Subst::apply(const Variable::ptr &term) {
  if (term->isConst())
    return term;
  if (term->isVariable()) {
    if (m_pairs.count(term->getValue()) > 0)
      return m_pairs[term->getValue()];
    // проверяем наличие переменной в связанном кольце неозначенных переменных
    for (auto &ring : m_links)
      if (ring.count(term->getValue()) > 0)
        return std::make_shared<Variable>(false, *ring.begin());
    return term;
  }
  // если term - функциональный символ - выполняем все остальное
  if (!term->hasVars())
    return term;
  auto name = term->getValue();
  std::vector<Variable::ptr> args;
  for (auto arg : term->getArguments())
    args.push_back(apply(arg));
  return std::make_shared<Variable>(false, std::move(name), std::move(args));
}

Atom Subst::apply(const Atom &atom) {
  std::vector<Variable::ptr> newArgs;
  for (const auto &arg : atom.getArguments())
    newArgs.push_back(apply(arg));
  return Atom(atom.getName(), std::move(newArgs));
}

std::string Subst::toString() const {
  std::set<std::string> accounted;
  std::string res = "{";
  bool first = true;
  for (const auto &ring : m_links) {
    if (!first)
      res += ", ";
    first = false;
    for (const auto &var : ring) {
      res += var + "=";
      accounted.insert(var);
    }
    if (m_pairs.count(*ring.begin()) > 0)
      res += m_pairs.at(*ring.begin())->toString();
  }
  for (const auto &[var, value] : m_pairs) {
    if (accounted.count(var) == 0) {
      if (!first)
        res += ", ";
      first = false;
      res += var + "=" + value->toString();
    }
  }
  res += "}";
  return res;
}

bool Subst::ringValid(const std::set<std::string> &ring) const {
  if (m_pairs.count(*ring.begin()) == 0) {
    // all other items also must not have value
    for (const auto &item : ring) {
      if (m_pairs.count(item) != 0)
        return false;
    }
    return true;
  }
  auto firstValue = m_pairs.at(*ring.begin())->toString();
  for (const auto &item : ring) {
    if (m_pairs.count(item) == 0)
      return false;
    if (m_pairs.at(item)->toString() != firstValue)
      return false;
  }
  return true;
}

void Subst::solveRecursion(Variable::ptr &value, int depth) {
  if (depth > 10)
    return;
  if (value->isFuncSym()) {
    auto args = value->getArguments();
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i]->isVariable()) {
        value->updateArgument(i, apply(args[i]));
      } else if (args[i]->isFuncSym()) {
        solveRecursion(args[i], depth + 1);
        value->updateArgument(i, args[i]);
      }
    }
  }
}
