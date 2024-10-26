#pragma once

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

class Rule : public std::enable_shared_from_this<Rule> {
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

  using ptr = std::shared_ptr<Rule>;

  // general constructor for operators
  Rule(Type type, std::vector<ptr> operands, std::set<std::string> vars = {});

  // constants
  Rule(std::true_type);
  Rule(std::false_type);

  // atom/predicate constructor
  Rule(std::string name, std::vector<ptr> operands = {});

  static ptr createTrue();
  static ptr createFalse();
  static ptr createAtom(std::string value);
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
  static ptr createImplication(ptr from, ptr to);
  static ptr createEquality(ptr left, ptr right);

  static ptr createExists(std::set<std::string> vars, Rule::ptr rule);
  static ptr createForAll(std::set<std::string> vars, Rule::ptr rule);

  bool operator==(const Rule &other) const;
  bool operator!=(const Rule &other) const;

  bool operator==(const std::string &val) const;
  bool operator!=(const std::string &val) const;

  std::string toString() const;

  ptr toNormalForm();

  // appliable only to rules in conjunctive normal form
  std::list<ptr> getDisjunctionsList() const;

  std::vector<ptr> getOperands() const { return operands; }

  // checks weither this rule contains free vars
  std::set<std::string> getFreeVars() const;

  // rename free variable in this rule globally
  void renameVariable(const std::string &oldName, const std::string &newName);

  friend bool contraryPair(const Rule &left, const Rule &right);

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
};

inline bool operator==(const std::string &str, const Rule &rule) {
  return rule == str;
}

inline bool operator!=(const std::string &str, const Rule &rule) {
  return rule != str;
}
