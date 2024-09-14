#pragma once

#include <list>
#include <map>
#include <string>
#include "rule.h"

class Dictionary
{
public:
    explicit Dictionary(const char* filename);

    const std::string& Fact(int node) const;
    const std::list<Rule>& Rules() const;

private:
    void Load(const char* filename);
    void AddRule(std::string precondition, std::string conclusion);

    std::map<int, std::string> m_facts;
    std::map<std::string, int> m_factsInverse;
    std::list<Rule> m_rules;
};
