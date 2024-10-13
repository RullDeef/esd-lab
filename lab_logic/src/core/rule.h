#pragma once

#include <memory>
#include <string>
#include <vector>

class Rule : public std::enable_shared_from_this<Rule> {
public:
  enum class Type {
    atom,
    inverse,
    disjunction,
    conjunction,
  };

  using ptr = std::shared_ptr<Rule>;

  // atom constructor
  Rule(std::string value);
  Rule(Type type, std::vector<ptr> operands);

  static ptr createAtom(std::string value);
  static ptr createInverse(ptr rule);
  static ptr createConjunction(ptr left, ptr right);
  static ptr createDisjunction(ptr left, ptr right);
  static ptr createImplication(ptr from, ptr to);
  static ptr createEquality(ptr left, ptr right);

  bool operator==(const Rule &other) const;
  bool operator!=(const Rule &other) const;

  bool operator==(const std::string &val) const;
  bool operator!=(const std::string &val) const;

  std::string toString() const;

  ptr toNormalForm();

private:
  ptr inverseToNormalForm();
  ptr conjunctionToNormalForm();
  ptr disjunctionToNormalForm();

  Type type;
  std::string value;
  std::vector<ptr> operands;
};

inline bool operator==(const std::string &str, const Rule &rule) {
  return rule == str;
}

inline bool operator!=(const std::string &str, const Rule &rule) {
  return rule != str;
}
