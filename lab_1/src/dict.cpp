#include <cstring>
#include <fstream>
#include <iostream>
#include "dict.h"

constexpr auto ruleStart = "Если ";
constexpr auto ruleMiddle = ", то ";
constexpr auto ruleEnd = ".";

Dictionary::Dictionary(const char* filename)
{
    Load(filename);
}

const std::string& Dictionary::Fact(int node) const
{
    return m_facts.at(node);
}

const std::list<Rule>& Dictionary::Rules() const
{
    return m_rules;
}

void Dictionary::Load(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("failed to open dictionary file");
    std::string line;
    int lineNum = 0;
    while (true) {
        std::getline(file, line, '\n');
        if (line.empty())
            break;
        ++lineNum;
        size_t startPos = line.find(ruleStart);
        size_t midPos = line.find(ruleMiddle);
        size_t endPos = line.find(ruleEnd);
        if (startPos == std::string::npos || midPos == std::string::npos || endPos == std::string::npos) {
            std::string msg = "invalid rule at line " + std::to_string(lineNum);
            throw std::runtime_error(std::move(msg));
        }
        std::string precondition = line.substr(startPos + strlen(ruleStart), midPos - startPos - strlen(ruleStart));
        std::string conclusion = line.substr(midPos + strlen(ruleMiddle), endPos - midPos - strlen(ruleMiddle));
        AddRule(std::move(precondition), std::move(conclusion));
    }
    for (const auto& [fact, node] : m_factsInverse)
        m_facts[node] = fact;
}

void Dictionary::AddRule(std::string precondition, std::string conclusion)
{
    if (m_factsInverse.count(precondition) == 0)
        m_factsInverse[precondition] = 1 + m_factsInverse.size();
    if (m_factsInverse.count(conclusion) == 0)
        m_factsInverse[conclusion] = 1 + m_factsInverse.size();
    Rule rule;
    rule.srcNode = m_factsInverse[precondition];
    rule.dstNode = m_factsInverse[conclusion];
    rule.number = 100 + m_rules.size();
    m_rules.push_back(rule);
}
