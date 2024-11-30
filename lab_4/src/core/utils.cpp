#include "utils.h"
#include <cctype>
#include <stdexcept>
#include <string>

std::string nextIndexed(const std::string &name) {
  if (name.empty())
    throw std::runtime_error("empty name for indexing");
  int lastPos = name.size() - 1;
  while (lastPos >= 0 && std::isalnum(name[lastPos]) &&
         !std::isalpha(name[lastPos]))
    lastPos--;
  if (lastPos == -1)
    throw std::runtime_error("numeric only name is invalid");
  unsigned long index = 1;
  if (lastPos < name.size() - 1)
    index += std::atoi(name.c_str() + lastPos + 1);
  return name.substr(0, lastPos + 1) + std::to_string(index);
}
