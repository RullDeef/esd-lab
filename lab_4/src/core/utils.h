#pragma once

#include <memory>
#include <string>

std::string nextIndexed(const std::string &name);

template <typename Ret, typename T> auto mem_ptr_fn(Ret (T::*mp)() const) {
  return [mp](const std::shared_ptr<T> &obj) -> auto {
    return (obj.get()->*mp)();
  };
}

template <typename Iter>
static std::string toStringSep(Iter begin, Iter end, const char *sep) {
  std::string res;
  bool first = true;
  while (begin != end) {
    if (!first)
      res += sep;
    first = false;
    res += *begin++;
  }
  return res;
}

template <typename Iter, typename Transform>
static std::string toStringSep(Iter begin, Iter end, const char *sep,
                               Transform func) {
  std::string res;
  bool first = true;
  while (begin != end) {
    if (!first)
      res += sep;
    first = false;
    res += func(*begin++);
  }
  return res;
}
