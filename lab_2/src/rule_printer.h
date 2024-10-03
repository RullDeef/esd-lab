#pragma once

#include "dict.h"
#include <memory>

class RulePrinter {
public:
  explicit RulePrinter(std::shared_ptr<Dictionary> dictionary);

  void PrintRule(Rule rule) const;

private:
  std::shared_ptr<Dictionary> m_dict;
};
