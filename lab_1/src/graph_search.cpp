#include "graph_search.h"

GraphSearch::GraphSearch(std::list<Rule> rules, int srcNode, int dstNode)
    : m_rules(std::move(rules)), m_dstNode(dstNode)
{
    for (auto rule : m_rules) {
        m_nodes[rule.srcNode] = { .number = rule.srcNode };
        m_nodes[rule.dstNode] = { .number = rule.dstNode };
    }
    m_openNodes.push_back(srcNode);
}

std::list<int> GraphSearch::DoDepthFirstSearch()
{
    while (!m_foundSolution && !m_noSolution) {
        /* достаем вершину из списка открытых вершин (стек) */
        int node = m_openNodes.back();
        /* вызываем метод потомки */
        int count = DescendantsDFS(node);
        /* Если установили флаг решения в потомках, то решение получено (путь в стеке) */
        if (m_foundSolution)
            break;
        else if (count == 0 && !m_openNodes.empty()) {
            m_nodes[node].forbidden = true;
            m_openNodes.pop_back();
        }
        else if (m_openNodes.empty())
            m_noSolution = true;
    }
    if (m_noSolution)
        return {};
    /* раскрутка стека решения */
    std::list<int> solution;
    int lastNode = m_dstNode;
    for (auto node = m_openNodes.rbegin(); node != m_openNodes.rend(); ++node) {
        for (auto rule : m_rules) {
            if (rule.srcNode == *node && rule.dstNode == lastNode) {
                solution.push_front(rule.number);
                break;
            }
        }
        lastNode = *node;
    }
    return solution;
}

int GraphSearch::DescendantsDFS(int node)
{
    int count = 0;
    for (auto& rule : m_rules) {
        if (rule.visited || rule.srcNode != node || m_nodes[rule.dstNode].forbidden)
            continue;
        m_openNodes.push_back(rule.dstNode);
        rule.visited = true;
        ++count;
        /* проверяем, что нашли решение */
        if (rule.dstNode == m_dstNode) {
            m_foundSolution = true;
            break;
        }
    }
    return count;
}

std::list<int> GraphSearch::DoBreadthFirstSearch()
{
    /*
    Алгоритм поиска:
    - пока флаги истинны, выполняем:
        - вызываем метод потомки
        - вершину из головы списка открытых вершин (у нас очередь) удаляем
        - проверяем, если число потомков не равно нулю, пишем эту вершину в список закрытых вершин
        - проверяем, если флаг решения 0 - выходим и выводим список закрытых вершин
        - иначе, если число потомков 0 и список открытых вершин пуст - то сбрасываем в 0
            флаг "нет решения" и выходим из цикла
    */
    while (!m_foundSolution && !m_noSolution) {
        int node = m_openNodes.front();
        int count = DescendantsBFS(node);
        if (m_foundSolution)
            break;
        m_openNodes.pop_front();
        if (count != 0)
            m_closedNodes.push_back(node);
        else if (m_openNodes.empty())
            m_noSolution = true;
    }
    if (m_noSolution)
        return {};
    /* формируем решение */
    int lastNode = m_openNodes.front();
    std::list<int> solution;
    for (auto& rule : m_rules) {
        if (rule.srcNode == lastNode && rule.dstNode == m_dstNode) {
            solution.push_front(rule.number);
            break;
        }
    }
    while (lastNode != m_closedNodes.front()) {
        for (auto node = m_closedNodes.begin(); *node != lastNode; ++node) {
            for (auto& rule : m_rules) {
                if (rule.visited && rule.srcNode == *node && rule.dstNode == lastNode) {
                    solution.push_front(rule.number);
                    lastNode = *node;
                    break;
                }
            }
            if (lastNode == *node)
                break;
        }
    }
    return solution;
}

int GraphSearch::DescendantsBFS(int node)
{
    /*
    Алгоритм поиска потомков:
    - пока не конец базы правил:
        - выбираем образец из головы списка открытых вершин
        - если образец равен цели, то флаг решения в 0,
        - иначе (находим инцидентные ребра, начальная вершина которых совпадает с образцом)
        - проверяем, что образец совпадает с начальной вершиной ребра, метка ребра равна нулю (оно еще не выбрано)
        - если конечная вершина равна цели, то флаг решения в 0, и можно выйти из цикла.
        - иначе - находим индидентное ребро, то есть те же условия, что были, только без цели
        - конечную вершину ребра записываем в хвост списка открытых
        - метку ребра ставим в 1
        - число потомков++
    все
    */
    int count = 0;
    for (auto& rule : m_rules) {
        if (rule.visited || rule.srcNode != node)
            continue;
        if (rule.dstNode == m_dstNode) {
            m_foundSolution = true;
            break;
        }
        m_openNodes.push_back(rule.dstNode);
        rule.visited = true;
        ++count;
    }
    return count;
}
