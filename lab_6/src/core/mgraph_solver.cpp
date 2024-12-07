#include "mgraph_solver.h"
#include "channel.h"
#include "name_allocator.h"
#include "solver.h"
#include "subst.h"
#include "variable.h"
#include <memory>
#include <thread>
#include <utility>

/**
 * Функция конвертации генератора с обычным типом подстановок (класс Subst) в
 * генератор с расширенным типом подстановок (класс SubstEx).
 *
 * Новый генератор выбрасывает очередное значение полученное от старого
 * генератора вместе со сброшенным флагом отсечения.
 */
static TaskChanPair<MGraphSolver::SubstEx>
taskChanPairEx(TaskChanPair<Subst> &&tcp) {
  // создаем канал для переброса подстановок
  auto chan = std::make_shared<Channel<MGraphSolver::SubstEx>>();
  // создаем отдельный поток для работы генератора
  std::jthread worker([tcp = std::move(tcp), chan]() {
    while (true) {
      // получить следующую подстановку
      auto [subst, ok] = tcp.second->get();
      if (!ok) {
        // если старый канал закрылся - закрываем новый
        chan->close();
        break;
      }
      // перебрасываем подстановку со сброшенным флагом отсечения
      if (!chan->put({std::move(subst), false}))
        break; // выходной канал закрылся с другого конца
    }
    tcp.second->close();
  });
  // возвращаем пару (поток, канал) представляющую генератор
  return std::make_pair(std::move(worker), std::move(chan));
}

/**
 * Функция переименования переменных в правиле с использованием переданного
 * аллокатора имен (класс NameAllocator).
 *
 * Не изменяет переданное правило, а создает на его основе новое правило.
 */
static Rule standardize(const Rule &rule, NameAllocator &allocator) {
  Atom output = rule.getOutput().renamedVars(allocator);
  std::vector<Atom> inputs;
  for (auto input : rule.getInputs())
    inputs.push_back(input.renamedVars(allocator));
  allocator.commit();
  return Rule(std::move(inputs), std::move(output));
}

/**
 * Метод обратного поиска в глубину.
 *
 * target - цель, которую необходимо доказать
 * output - канал для передачи результирующих подстановок
 *
 * Запускает метод generateOr и перехватывает генерируемые им подстановки для
 * того, чтобы отфильтровать те переменные, которые не упоминаются в заданной
 * цели target.
 */
void MGraphSolver::solveBackwardThreaded(Atom target, Channel<Subst> &output) {
  auto [worker, mid] = generateOr(target, Subst(), NameAllocator());
  while (!output.isClosed()) {
    auto [substEx, ok] = mid->get();
    // если канал mid был закрыт, то завершаем работу
    if (!ok) {
      output.close();
      break;
    }
    // фильтрация переменных, не относящихся к цели target
    Subst filtered;
    for (auto varName : target.getAllVars())
      filtered.insert(varName,
                      substEx.subst.apply(Variable::createVariable(varName)));
    // если выходной канал закрыли с другого конца, то завершаем работу
    if (!output.put(std::move(filtered)))
      break;
  }
  mid->close();
}

/**
 * Метод обратного поиска ИЛИ.
 *
 * target - текущая цель, которую необходимо доказать
 * baseSubst - накопленная подстановка к моменту вызова этого метода
 * allocator - контейнер использованных имен (для переименования переменных в
 * правилах)
 *
 * Этот метод является прослойкой перед настоящим методом поиска ИЛИ по базе
 * правил. Он нужен для прозрачной реализации специальных процедур,
 * доказательство которых происходит в обход базы правил.
 *
 * Этот метод производит поиск обработчика специальной процедуры в таблице
 * специальных процедур m_atomHooks. В случае успешного нахождения обработчика -
 * вызывает его для доказательства цели, иначе - передает управление настоящему
 * методу поиска ИЛИ.
 */
TaskChanPair<MGraphSolver::SubstEx>
MGraphSolver::generateOr(Atom target, Subst baseSubst,
                         NameAllocator allocator) {
  // поиск обработчика специальной процедуры по имени предиката цели
  if (m_atomHooks.count(target.getName()) == 0)
    // обработчика нет - вызываем настоящий метод поиска для обхода базы правил
    return generateOrBasic(std::move(target), std::move(baseSubst),
                           std::move(allocator));
  auto hook = m_atomHooks.at(target.getName());
  // вызов обработчика для доказательства цели в обход базы правил
  auto tcp = hook->prove(target.getArguments(), std::move(baseSubst));
  // конвертация генератора обычных подстановок в расширенные, со сброшенным
  // флагом отсечения
  return taskChanPairEx(std::move(tcp));
}

/**
 * Настоящий метод обратного поиска ИЛИ.
 *
 * target - текущая цель, которую необходимо доказать
 * baseSubst - накопленная подстановка к моменту вызова метода
 * allocator - контейнер использованных имен (для переименования переменных в
 * правилах)
 *
 * Производит обход базы правил, выбирая правила, которые могут доказать цель.
 * Для каждого такого правила вызывается метод поиска И для доказательства всех
 * подцелей из атницидента правила.
 *
 * В случае обнаружения сигнала об отсечении (установленный флаг cut в структуре
 * SubstEx) - дальнейший перебор правил прекращается.
 */
TaskChanPair<MGraphSolver::SubstEx>
MGraphSolver::generateOrBasic(Atom target, Subst baseSubst,
                              NameAllocator allocator) {
  // создаем канал для передачи генерируемых подстановок
  auto output = std::make_shared<Channel<SubstEx>>();
  // создаем отдельный поток для работы генератора
  std::jthread worker([this, target = std::move(target),
                       baseSubst = std::move(baseSubst),
                       allocator = std::move(allocator), output]() {
    for (auto &rule : m_database->getRules()) {
      // если выходной канал закрыли с другого конца, то завершаем работу
      if (output->isClosed()) {
        break;
      }
      // копируем контейнер имен и базовую подстановку, чтобы иметь возможность
      // вернуться к исходному состоянию при переходе к следующему правилу базы
      // правил (можно сказать, для отката)
      NameAllocator subAllocator = allocator;
      Subst subst = baseSubst;
      // выполняем переименование переменных в правиле
      Rule stdRule = standardize(rule, subAllocator);
      // выполняем унификацию цели с выходом правила
      if (!unify(target, stdRule.getOutput(), subst))
        continue; // унификация неуспешна - переходим к следующему правилу
      // если правило на самом деле факт (нет входов), то выбрасываем текущую
      // подстановку (передаем через выходной канал) и переходим к следующему
      // правилу в базе.
      //
      // В книге эта проверка не делается, а передается пустой список подцелей в
      // метод поиска И, который уже выбросит эту же подстановку.
      if (stdRule.isFact()) {
        // если выходной канал закрыли с другого конца, то завершаем работу
        if (!output->put({subst, false}))
          break;
        // переходим к следующему правилу в базе правил
        continue;
      }
      // формируем список подцелей, применяя текущую подстановку ко всем входам
      // правила
      std::vector<Atom> subGoals;
      for (auto &atom : stdRule.getInputs())
        subGoals.push_back(subst.apply(atom));
      // вызываем метод поиска И для списка подцелей. К следующему правилу не
      // перейдем, пока не обработаем все подстановки, которые будут
      // сгенерированы здесь. Поэтому получается поиск в глубину
      auto [worker, mid] = generateAnd(std::move(subGoals), std::move(subst),
                                       std::move(subAllocator));
      bool wasCut = false; // флаг обнаружения отсечения
      while (true) {
        // ожидаем следующую подстановку из канала
        auto [subst2, ok] = mid->get();
        // если канал закрылся, то больше подстановок не будет. Завершаем
        // внутренний цикл
        if (!ok)
          break;
        if (subst2.cut) {
          // обнаружено отсечение - запретить дальнейший перебор правил из базы
          wasCut = true;
        } else if (!output->put({std::move(subst2.subst), false})) {
          // выходной канал закрыли с другого конца - закрываем промежуточный
          // канал (прерываем работу генератора И) и выходим
          mid->close();
          return;
        }
      }
      mid->close();
      // прекратить дальнейший перебор правил, если было обнаружено отсечение
      if (wasCut)
        break;
    }
    // все правила просмотрены или встречено отсечение - больше подстановок
    // сгенерировано не будет, поэтому закрываем выходной канал
    output->close();
  });
  // возвращаем пару (поток, канал) представляющую генератор
  return std::make_pair(std::move(worker), output);
}

/**
 * Метод обратного поиска И.
 *
 * targets - список целей для доказательства
 * baseSubst - накопленная подстановка к моменту вызова метода
 * allocator - контейнер использованных имен (для переименования переменных в
 * правилах)
 *
 * Если список целей пуст - выбрасывает накопленную подстановку.
 * Иначе если первая цель в списке является отсечением - выбрасывает пустую
 * подстановку с установленным флагом отсечения (которая обрабатывается в методе
 * поиска ИЛИ) и вызывает рекурсивно метод поиска И для оставшихся целей в
 * списке.
 * Иначе - вызывает метод поиска ИЛИ для первой цели в списке и для каждой
 * сгенерированной подстановки вызывает рекурсивно метод поиска И для
 * доказательства оставшихся целей в списке.
 */
TaskChanPair<MGraphSolver::SubstEx>
MGraphSolver::generateAnd(std::vector<Atom> targets, Subst baseSubst,
                          NameAllocator allocator) {
  // создаем канал для передачи генерируемых подстановок
  auto output = std::make_shared<Channel<SubstEx>>();
  // создаем отдельный поток для работы генератора
  std::jthread worker([this, targets = std::move(targets),
                       baseSubst = std::move(baseSubst),
                       allocator = std::move(allocator), output]() {
    if (targets.empty()) {
      // если список целей пуст, то выбрасываем текущую подстановку и закрываем
      // канал
      output->put({baseSubst, false});
      output->close();
      return;
    }
    Subst subst = baseSubst;
    // разбить список целей на голову и хвост, применив к ним текущую
    // подстановку
    Atom first = subst.apply(targets.front());
    std::vector<Atom> rest(targets.begin() + 1, targets.end());
    for (auto &atom : rest)
      atom = subst.apply(atom);
    // если первая цель в списке - отсечение, то выбрасываем специальную
    // подстановку с флагом отсечения и рекурсивно обрабатываем оставшиеся цели
    if (first.toString() == "cut" || first.toString() == "!") {
      output->put({{}, true});
      auto [worker, andChan] = generateAnd(rest, subst, std::move(allocator));
      while (true) {
        // перебрасываем подстановки из промежуточного канала в выходной
        auto [substEx2, ok2] = andChan->get();
        // если промежуточный канал закрыт с другого конца - больше подстановок
        // не будет, завершаем работу
        if (!ok2)
          break;
        if (!output->put({std::move(substEx2.subst), false})) {
          // если выходной канал закрыли с другого конца - прекращаем работу
          andChan->close();
          return;
        }
      }
    } else {
      // первая цель в списке не является отсечением - вызываем метод поиска ИЛИ
      auto [orWorker, orChan] = generateOr(std::move(first), subst, allocator);
      while (true) {
        // получаем следующую подстановку из промежуточного канала
        auto [substEx, ok] = orChan->get();
        if (!ok)
          break; // канал закрыт, больше подстановок не будет
        // формируем контейнер использованных имен переменных на основе
        // полученной подстановки
        NameAllocator subAllocator = allocator;
        for (auto &name : substEx.subst.getAllVarNames())
          subAllocator.allocateName(name);
        // рекурсивный вызов метода поиска И для оставшихся подцелей
        auto [andWorker, andChan] =
            generateAnd(rest, substEx.subst, std::move(subAllocator));
        while (true) {
          // перебрасывание генерируемых подстановок из промежуточного канала в
          // выходной
          auto [substEx2, ok2] = andChan->get();
          if (!ok2)
            break; // канал закрылся, больше подстановок не будет
          if (!output->put(std::move(substEx2))) {
            // выходной канал закрыли с другого конца - завершаем работу
            andChan->close();
            orChan->close();
            return;
          }
        }
        andChan->close();
      }
      orChan->close();
    }
    output->close();
  });
  // возвращаем пару (поток, канал) представляющую генератор
  return std::make_pair(std::move(worker), output);
}
