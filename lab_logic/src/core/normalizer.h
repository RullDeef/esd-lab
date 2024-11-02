#pragma once

#include "parser/expr.h"
#include "resolver_new.h"
#include "utils.h"
#include <cctype>
#include <stdexcept>

class ExprNormalizer {
public:
  std::vector<Disjunct> scolemForm(Expr::ptr expr) {
    expr = normalForm(std::move(expr));
    std::vector<Disjunct> disjunctions;

    std::vector<std::string> vars;
    while (expr->getType() == Expr::Type::exists ||
           expr->getType() == Expr::Type::forall) {
      Expr::ptr next = expr->getOperands()[0];
      if (expr->getType() == Expr::Type::exists) {
        for (auto var : expr->getVars()) {
          next = next->withReplacedVariable(
              var, Expr::createTerm("F" + std::to_string(m_funcCounter), vars));
          ++m_funcCounter;
        }
      } else {
        auto exprVars = expr->getVars();
        vars.insert(vars.end(), exprVars.begin(), exprVars.end());
      }
      expr = std::move(next);
    }
    for (const auto &disjExpr : expr->getOperands()) {
      std::vector<Atom> atoms;
      for (const auto &atomExpr : disjExpr->getOperands())
        atoms.push_back(transformAtomExpr(atomExpr));
      disjunctions.push_back(std::move(atoms));
    }
    return disjunctions;
  }

private:
  Expr::ptr normalForm(Expr::ptr expr) {
    switch (expr->getType()) {
    case Expr::Type::constant:
      return std::move(expr);
    case Expr::Type::atom:
      return std::make_shared<Expr>(
          Expr::Type::conjunction,
          std::vector{std::make_shared<Expr>(Expr::Type::disjunction,
                                             std::vector{std::move(expr)})});
    case Expr::Type::inverse:
      return inverseToNormalForm(std::move(expr));
    case Expr::Type::conjunction:
      return conjunctionToNormalForm(std::move(expr));
    case Expr::Type::disjunction:
      return disjunctionToNormalForm(std::move(expr));
    case Expr::Type::exists:
    case Expr::Type::forall:
      return quantifierToNormalForm(std::move(expr));
    }
  }

  Expr::ptr inverseToNormalForm(Expr::ptr expr) {
    auto operands = expr->getOperands();
    switch (operands[0]->getType()) {
    case Expr::Type::constant:
      if (operands[0]->getValue() == "1")
        return Expr::createFalse();
      else
        return Expr::createTrue();
    case Expr::Type::atom:
      return std::make_shared<Expr>(
          Expr::Type::conjunction,
          std::vector{std::make_shared<Expr>(Expr::Type::disjunction,
                                             std::vector{std::move(expr)})});
    case Expr::Type::inverse:
      return normalForm(operands[0]->getOperands()[0]);
    case Expr::Type::disjunction:
      return normalForm(Expr::createConjunction(
          Expr::createInverse(operands[0]->getOperands()[0]),
          Expr::createInverse(operands[0]->getOperands()[1])));
    case Expr::Type::conjunction:
      return normalForm(Expr::createDisjunction(
          Expr::createInverse(operands[0]->getOperands()[0]),
          Expr::createInverse(operands[0]->getOperands()[1])));
    case Expr::Type::exists:
      return normalForm(Expr::createForAll(
          operands[0]->getVars(),
          Expr::createInverse(operands[0]->getOperands()[0])));
    case Expr::Type::forall:
      return normalForm(Expr::createExists(
          operands[0]->getVars(),
          Expr::createInverse(operands[0]->getOperands()[0])));
    }
  }

  Expr::ptr conjunctionToNormalForm(Expr::ptr expr) {
    auto operands = expr->getOperands();
    auto left = normalForm(operands[0]);
    auto right = normalForm(operands[1]);

    // constant propagation
    if (left->getType() == Expr::Type::constant)
      return left->getValue() == "0" ? left : right;
    if (right->getType() == Expr::Type::constant)
      return right->getValue() == "0" ? right : left;

    // extract all quantifiers from both operands (with optional renaming)
    std::map<std::string, std::string> renamings;
    std::vector<std::pair<Expr::Type, std::set<std::string>>> quantifiers;
    left = extractFrontQuantifiers(renamings, quantifiers, left);
    right = extractFrontQuantifiers(renamings, quantifiers, right);

    // drag all elementary disjunctions from both paths
    auto disjunctions = left->getOperands();
    auto rightOps = right->getOperands();
    std::copy_if(rightOps.begin(), rightOps.end(),
                 std::back_inserter(disjunctions), [&disjunctions](auto x) {
                   return std::all_of(disjunctions.begin(), disjunctions.end(),
                                      [&x](auto d) { return *x != *d; });
                 });
    Expr::ptr res =
        std::make_shared<Expr>(Expr::Type::conjunction, disjunctions);

    // apply quantifiers if any
    for (int i = quantifiers.size() - 1; i >= 0; --i) {
      if (quantifiers[i].first == Expr::Type::exists)
        res = Expr::createExists(quantifiers[i].second, res);
      else
        res = Expr::createForAll(quantifiers[i].second, res);
    }

    return res;
  }

  Expr::ptr disjunctionToNormalForm(Expr::ptr expr) {
    auto operands = expr->getOperands();
    auto left = normalForm(operands[0]);
    auto right = normalForm(operands[1]);

    // constants propagation
    if (left->getType() == Expr::Type::constant)
      return left->getValue() == "1" ? left : right;
    if (right->getType() == Expr::Type::constant)
      return right->getValue() == "1" ? left : right;

    // extract all quantifiers
    std::map<std::string, std::string> renamings;
    std::vector<std::pair<Expr::Type, std::set<std::string>>> quantifiers;
    left = extractFrontQuantifiers(renamings, quantifiers, left);
    right = extractFrontQuantifiers(renamings, quantifiers, right);

    auto left_dsj = left->getOperands();
    auto right_dsj = right->getOperands();
    std::vector<Expr::ptr> disjunctions;
    for (size_t i = 0; i < left_dsj.size(); ++i) {
      for (size_t j = 0; j < right_dsj.size(); ++j) {
        auto args = left_dsj[i]->getOperands();
        auto right_dsj_ops = right_dsj[j]->getOperands();
        std::copy_if(right_dsj_ops.begin(), right_dsj_ops.end(),
                     std::back_inserter(args), [&args](auto x) {
                       return std::all_of(
                           args.begin(), args.end(),
                           [&x](const Expr::ptr &d) { return *x != *d; });
                     });
        auto newRule = std::make_shared<Expr>(Expr::Type::disjunction, args);
        if (std::all_of(disjunctions.begin(), disjunctions.end(),
                        [&newRule](auto d) { return *d != *newRule; }))
          disjunctions.push_back(newRule);
      }
    }

    // check contrary disjunctions
    for (int i = disjunctions.size() - 1; i >= 0; --i) {
      disjunctions[i] = disjunctReduction(disjunctions[i]);
      if (disjunctions[i]->getType() == Expr::Type::constant)
        disjunctions.erase(disjunctions.begin() + i);
    }
    if (disjunctions.empty())
      return Expr::createTrue();

    auto res = std::make_shared<Expr>(Expr::Type::conjunction, disjunctions);

    // apply quantifiers if any
    for (int i = quantifiers.size() - 1; i >= 0; --i) {
      if (quantifiers[i].first == Expr::Type::exists)
        res = Expr::createExists(quantifiers[i].second, res);
      else
        res = Expr::createForAll(quantifiers[i].second, res);
    }

    return res;
  }

  Expr::ptr quantifierToNormalForm(Expr::ptr expr) {
    auto rule = normalForm(expr->getOperands()[0]);
    auto ruleVars = rule->getFreeVars();
    std::set<std::string> newVars;
    for (const auto &var : expr->getVars())
      if (ruleVars.count(var) > 0)
        newVars.insert(var);
    if (newVars.empty())
      return rule;
    return std::make_shared<Expr>(expr->getType(), std::vector{rule}, newVars);
  }

  Expr::ptr extractFrontQuantifiers(
      std::map<std::string, std::string> &renamings,
      std::vector<std::pair<Expr::Type, std::set<std::string>>> &quantifiers,
      Expr::ptr rule) {
    if (renamings.empty()) {
      while (rule->getType() == Expr::Type::forall ||
             rule->getType() == Expr::Type::exists) {
        for (const auto &var : rule->getVars())
          renamings[var] = var;
        quantifiers.emplace_back(rule->getType(), rule->getVars());
        rule = rule->getOperands()[0];
      }
    } else {
      while (rule->getType() == Expr::Type::forall ||
             rule->getType() == Expr::Type::exists) {
        // check for renaming
        std::set<std::string> newVars;
        auto ruleVars = rule->getVars();
        auto next = rule->getOperands()[0];
        for (auto var : ruleVars) {
          int count = renamings.count(var);
          if (count == 0) {
            renamings[var] = var;
            newVars.insert(var);
          } else {
            auto newName = nextIndexed(var);
            while (renamings.count(newName) > 0 && ruleVars.count(newName) > 0)
              newName = nextIndexed(newName);
            renamings[newName] = var;
            next = next->withRenamedVariable(var, newName);
            newVars.insert(newName);
          }
        }
        if (!newVars.empty())
          quantifiers.emplace_back(rule->getType(), std::move(newVars));
        rule = std::move(next);
      }
    }
    return rule;
  }

  Expr::ptr disjunctReduction(Expr::ptr expr) {
    std::vector<Expr::ptr> terms = expr->getOperands();
    for (int i = terms.size() - 1; i >= 0; --i) {
      for (int j = i - 1; j >= 0; --j) {
        if (terms[i]->toString() == terms[j]->toString()) {
          terms.erase(terms.begin() + i);
          break;
        } else if (contraryPair(*terms[i], *terms[j]))
          return Expr::createTrue();
      }
    }
    return std::make_shared<Expr>(Expr::Type::disjunction, terms);
  }

  Atom transformAtomExpr(Expr::ptr expr) {
    if (expr->getType() == Expr::Type::inverse) {
      auto res = transformAtomExpr(expr->getOperands()[0]);
      return Atom(true, res.getName(), res.getArguments());
    }
    if (expr->getType() != Expr::Type::atom)
      throw std::runtime_error("invalid expr type");
    std::vector<Variable::ptr> operands;
    for (auto op : expr->getOperands())
      operands.push_back(transformVarExpr(op));
    return Atom(false, expr->getValue(), std::move(operands));
  }

  Variable::ptr transformVarExpr(Expr::ptr expr) {
    auto exprOps = expr->getOperands();
    if (exprOps.empty()) {
      char firstSym = expr->getValue()[0];
      bool isConst = !std::isalpha(firstSym) || !std::isupper(firstSym);
      return std::make_shared<Variable>(isConst, expr->getValue());
    }
    std::vector<Variable::ptr> args;
    for (auto arg : expr->getOperands())
      args.push_back(transformVarExpr(std::move(arg)));
    return std::make_shared<Variable>(false, expr->getValue(), std::move(args));
  }

  int m_funcCounter = 0;
};
