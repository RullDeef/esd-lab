#include "name_allocator.h"
#include <algorithm>
#include <map>
#include <stdexcept>

bool NameAllocator::allocateName(std::string name) {
  if (name == "_")
    return true;
  auto [base, index] = splitIndexed(std::move(name));
  if (m_allocated.count(base) == 0) {
    m_allocated[base] = std::list{index};
    return true;
  }
  auto &indexList = m_allocated[base];
  auto pos = std::lower_bound(indexList.begin(), indexList.end(), index);
  if (pos != indexList.end() && *pos == index)
    return false;
  indexList.insert(pos, index);
  return true;
}

std::string NameAllocator::allocateRenaming(std::string original) {
  if (original == "_")
    return original;
  if (m_working.count(original) != 0)
    return m_working[original];
  auto [base, index] = splitIndexed(original);
  if (m_allocated.count(base) == 0) {
    auto newName = joinIndexed(base, ++index);
    m_allocated[base] = std::list{index};
    m_working[original] = newName;
    return newName;
  }
  auto &indexList = m_allocated[base];
  std::list<int>::iterator pos;
  do {
    pos = std::lower_bound(indexList.begin(), indexList.end(), ++index);
  } while (pos != indexList.end() && *pos == index);
  indexList.insert(pos, index);
  auto newName = joinIndexed(base, index);
  m_working[original] = newName;
  return newName;
}

void NameAllocator::commit() { m_working.clear(); }

std::string NameAllocator::toString() const {
  std::string res = "{";
  bool first = true;
  for (auto &[base, indexList] : m_allocated) {
    if (!first)
      res += " ";
    first = false;
    res += base + ":";
    for (auto &index : indexList)
      res += std::to_string(index) + ";";
  }
  res += "}";
  if (!m_working.empty()) {
    res += "[";
    for (auto &[key, val] : m_working) {
      res += key + ":" + val + ",";
    }
    res += "]";
  }
  return res;
}

std::pair<std::string, int> NameAllocator::splitIndexed(std::string name) {
  if (name.empty())
    throw std::runtime_error("empty name for indexing");
  int lastPos = name.size() - 1;
  while (lastPos >= 0 && std::isalnum(name[lastPos]) &&
         !std::isalpha(name[lastPos]))
    lastPos--;
  if (lastPos == -1)
    throw std::runtime_error("numeric only name is invalid");
  int index = 0;
  if (lastPos < name.size() - 1)
    index = std::atoi(name.c_str() + lastPos + 1);
  return std::make_pair(name.substr(0, lastPos + 1), index);
}

std::string NameAllocator::joinIndexed(std::string name, int index) {
  return name + std::to_string(index);
}
