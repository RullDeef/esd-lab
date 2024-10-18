#pragma once

#include "rule.h"

class RuleParser {
public:
  static Rule::ptr Parse(const char *str);
};
