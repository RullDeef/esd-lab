#pragma once

#include <list>
#include <map>
#include <string>

// класс для хранения использованных имен переменных
class NameAllocator {
public:
  // добавляет имя в таблицу использованных имен
  bool allocateName(std::string name);
  // возвращает переименованную переменную
  std::string allocateRenaming(std::string original);
  // фиксирует все переименования, сделанные методом allocateRenaming
  void commit();

  std::string toString() const;

private:
  // разделить имя переменной на основу и номер, например x13 -> (x, 13)
  std::pair<std::string, int> splitIndexed(std::string name);

  // соединить основу и номер в имя переменной
  std::string joinIndexed(std::string name, int index);

  std::map<std::string, std::list<int>> m_allocated;
  std::map<std::string, std::string> m_working;
};
