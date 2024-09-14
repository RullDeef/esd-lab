#pragma once

#include <memory>
#include "dict.h"

class RulePrinter
{
public:
    explicit RulePrinter(std::shared_ptr<Dictionary> dictionary);

    void PrintRule(Rule rule) const;

private:
    std::shared_ptr<Dictionary> m_dict;
};
