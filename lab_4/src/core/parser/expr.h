#pragma once

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

class Expr : public std::enable_shared_from_this<Expr> {
public:
  enum class Type {
    constant,
    atom, // predicate actually
    inverse,
    disjunction,
    conjunction,
    forall, // value used for variable name
    exists, // value used for variable name
  };

  using ptr = std::shared_ptr<Expr>;

  // general constructor for operators
  Expr(Type type, std::vector<ptr> operands, std::set<std::string> vars = {});

  // constants
  Expr(std::true_type);
  Expr(std::false_type);

  // atom/predicate constructor
  Expr(std::string name, std::vector<ptr> operands = {});

  static ptr createTrue();
  static ptr createFalse();
  static ptr createAtom(std::string value);
  static ptr createTerm(std::string func, std::vector<std::string> vars);
  static ptr createPredicate(std::string name, std::vector<ptr> operands);
  static ptr createInverse(ptr rule);
  static ptr createConjunction(ptr left, ptr right);
  template <typename Iter> static ptr createConjunction(Iter begin, Iter end) {
    auto first = *begin++;
    return begin == end ? std::move(first)
                        : createConjunction(std::move(first),
                                            createConjunction(begin, end));
  }
  static ptr createDisjunction(ptr left, ptr right);
  template <typename Iter> static ptr createDisjunction(Iter begin, Iter end) {
    auto first = *begin++;
    return begin == end ? std::move(first)
                        : createDisjunction(std::move(first),
                                            createDisjunction(begin, end));
  }
  static ptr createImplication(ptr from, ptr to);
  static ptr createEquality(ptr left, ptr right);

  static ptr createExists(std::set<std::string> vars, Expr::ptr rule);
  static ptr createForAll(std::set<std::string> vars, Expr::ptr rule);

  bool operator==(const Expr &other) const;
  bool operator!=(const Expr &other) const;

  bool operator==(const std::string &val) const;
  bool operator!=(const std::string &val) const;

  std::string toString() const;

  ptr toNormalForm();
  ptr toScolemForm(int *replacementCounter = nullptr);

  // appliable only to rules in conjunctive normal form
  std::list<ptr> getDisjunctionsList() const;

  Type getType() const { return type; }
  const std::string &getValue() const { return value; }
  const std::set<std::string> &getVars() const { return vars; }
  const std::vector<ptr> &getOperands() const { return operands; }

  // checks weither this rule contains free vars
  std::set<std::string> getFreeVars() const;

  // rename free variable in this rule globally
  ptr withRenamedVariable(const std::string &oldName,
                          const std::string &newName);
  ptr withReplacedVariable(const std::string &varName, ptr term);

  friend bool contraryPair(const Expr &left, const Expr &right);

private:
  ptr inverseToNormalForm();
  ptr conjunctionToNormalForm();
  ptr disjunctionToNormalForm();
  ptr quantifierToNormalForm();

  // extracts quantifiers at front with possible renamings
  ptr extractFrontQuantifiers(
      std::map<std::string, std::string> &renamings,
      std::vector<std::pair<Type, std::set<std::string>>> &quantifiers,
      ptr rule);

  Type type;
  std::string value;
  // linked variables for quantifiers only
  std::set<std::string> vars;
  // for predicate this array holds references to variables and values which are
  // arguments for this predicate. For actual atom it is empty
  std::vector<ptr> operands;
  bool isCNF = false;

  friend class Substitution;
};

inline bool operator==(const std::string &str, const Expr &rule) {
  return rule == str;
}

inline bool operator!=(const std::string &str, const Expr &rule) {
  return rule != str;
}
