#pragma once

#include <list>
#include <map>
#include <string>

class NameAllocator {
public:
  bool allocateName(std::string name);
  // commit must be called in order to apply all changes made with this method
  std::string allocateRenaming(std::string original);
  void commit();

  void deallocate(std::string name);

  std::string toString() const;

private:
  std::pair<std::string, int> splitIndexed(std::string name);
  std::string joinIndexed(std::string name, int index);

  std::map<std::string, std::list<int>> m_allocated;
  std::map<std::string, std::string> m_working;
};
