#include "solver.h"
#include "channel.h"
#include "database.h"
#include "subst.h"
#include <chrono>
#include <deque>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <thread>

Solver::Solver(const Database &database) : m_database(database) {}

Solver::~Solver() { done(); }

void Solver::solveForward(Atom target) {
  done();
  m_channel = std::make_shared<Channel<Subst>>();
  m_solverThread = std::thread([this, target = std::move(target)]() {
    solveForwardThreaded(std::move(target), *m_channel);
    m_channel->close();
  });
}

void Solver::solveBackward(Atom target) {
  done();
  m_channel = std::make_shared<Channel<Subst>>();
  m_solverThread = std::thread([this, target = std::move(target)]() {
    solveBackwardThreaded(std::move(target), *m_channel);
    m_channel->close();
  });
}

std::optional<Subst> Solver::next() {
  if (!m_channel || m_channel->isClosed())
    return std::nullopt;
  auto [subst, ok] = m_channel->get();
  return ok ? std::optional(subst) : std::nullopt;
}

void Solver::done() {
  if (m_solverThread.joinable()) {
    m_channel->close();
    m_solverThread.join();
  }
}

void Solver::solveForwardThreaded(Atom target, Channel<Subst> &output) {
  WorkingDataset workset;
  for (auto &rule : m_database.getRules()) {
    if (!rule.isFact())
      continue;
    // проверить, находится ли цель среди фактов в базе правил
    Subst subst;
    if (unify(rule.getOutput(), target, subst)) {
      bool wasEmpty = subst.empty();
      output.put(std::move(subst));
      // завершить поиск если была сгенерирована пустая подстановка
      if (wasEmpty)
        return;
    }
    workset.addFact(rule.getOutput());
  }
  bool newAdded = true;
  while (newAdded) {
    newAdded = false;
    workset.nextIteration(); // обновить счетчик шага
    for (auto &rule : m_database.getRules()) {
      // проверить, что входы правила содержат атом из доказанных на
      // предыдущем шаге
      bool hasNewFacts = false;
      for (auto &input : rule.getInputs())
        if ((hasNewFacts = workset.hasNewFactFor(input)))
          break;
      // пропустить правило, если для него нет новых фактов
      if (!hasNewFacts)
        continue;
      // проверить покрытие входов из доказанных фактов
      auto channel = unifyInputs(rule, workset);
      while (true) {
        // перебираем все возможные подстановки
        auto [subst, ok] = channel->get();
        if (!ok)
          break;
        auto newFact = applySubst(subst, rule.getOutput());
        // проверить, что факт действительно новый
        if (workset.hasFact(newFact))
          continue;
        // проверить, что новый факт удовлетворяет цели
        Subst res;
        if (unify(newFact, target, res))
          if (!output.put(std::move(res)))
            break;
        workset.addFact(newFact);
        newAdded = true;
      }
    }
  }
}

void Solver::solveBackwardThreaded(Atom target, Channel<Subst> &output) {
  // стек резольвент
  std::deque<std::list<Atom>> resolventStack;
  // стек меток для перебора правил базы знаний
  std::deque<std::list<Rule>::const_iterator> labelStack;
  // стек отката
  std::deque<Atom> backtrackStack;
  // стек подстановок (переменных)
  std::deque<Subst> varsStack;

  size_t targetCounter = 1;

  labelStack.push_front(m_database.getRules().begin());
  resolventStack.push_front(std::list{target});
  varsStack.push_front({});
  while (!resolventStack.empty()) {
    std::cout << "=============================================\n";
    std::cout << "stacks:\n";
    std::cout << "  resolvents:\n";
    for (auto &res : resolventStack) {
      std::cout << "    {";
      for (auto &atom : res)
        std::cout << atom.toString() << ", ";
      std::cout << "}\n";
    }
    std::cout << "  labels:\n";
    for (auto &label : labelStack)
      std::cout << "    " << std::distance(m_database.getRules().begin(), label)
                << std::endl;
    std::cout << "  backtrack:\n";
    for (auto &atom : backtrackStack)
      std::cout << "    " << atom.toString() << std::endl;
    std::cout << "  substs:\n";
    for (auto &subst : varsStack)
      std::cout << "    " << subst.toString() << std::endl;
    std::cout << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    std::list<Atom> &currTargets = resolventStack.front();
    // если все подцели доказаны - выталкиваем их из стека
    if (currTargets.empty()) {
      resolventStack.pop_front();
      // TODO: update subst and other stacks
      std::cout << "currTargets empty!\n";
      continue;
    }
    // доказываем текущую подцель
    const Atom &currTarget = currTargets.back();
    std::cout << "curr target: " << currTarget.toString() << std::endl;
    // начинаем поиск правила, которое может доказать текущую подцель
    auto &ruleLabel = labelStack.front();
    bool currTargetResolved = false;
    while (!currTargetResolved && ruleLabel != m_database.getRules().end()) {
      std::cout << "checking rule " << ruleLabel->toString() << std::endl;
      Subst subst = varsStack.front();
      if (unify(currTarget, ruleLabel->getOutput(), subst)) {
        // переложить атом из верхушки стека резольвенты в стек отката
        backtrackStack.push_front(currTarget);
        currTargets.pop_back();
        varsStack.push_front(subst);
        targetCounter += ruleLabel->getInputs().size() - 1;
        // добавить входы правила в стек резольвенты, если есть
        if (ruleLabel->isFact()) {
          if (targetCounter == 0) {
            std::cout << "generating output!: " << subst.toString()
                      << std::endl;
            Subst filtered;
            for (auto &arg : target.getArguments()) {
              if (arg != "_" && isVar(arg)) {
                std::cout << "applying subst to arg " << arg << std::endl;
                filtered.insert(arg, varsStack.back().apply(arg));
              }
            }
            if (!output.put(filtered)) {
              return;
            }
            std::cout << "solution posted!\n";
          }
        } else {
          auto &inputs = ruleLabel->getInputs();
          resolventStack.emplace_front(inputs.rbegin(), inputs.rend());
          // применить подстановку к новым подцелям
          for (auto &atom : resolventStack.front())
            atom = applySubst(subst, atom);
          for (auto &atom : inputs)
            labelStack.push_front(m_database.getRules().begin());
        }
        currTargetResolved = true;
        std::cout << "resolved! " << subst.toString() << std::endl;
      }
      ruleLabel++;
    }
    if (!currTargetResolved) {
      // откат
      if (backtrackStack.empty()) {
        // доказательство текущей цели невозможно - перебрали все возможные
        // варианты
        return;
      } else {
        targetCounter++;
        auto &btrackAtoms = backtrackStack.front();
        for (auto &atom : resolventStack.front()) {
          targetCounter--;
          labelStack.pop_front();
        }
        resolventStack.pop_front();
        varsStack.pop_front();
        resolventStack.front().push_back(btrackAtoms);
        backtrackStack.pop_front();
      }
    }
  }
}

std::shared_ptr<Channel<Subst>> Solver::unifyInputs(const Rule &rule,
                                                    WorkingDataset &workset) {
  auto channel = std::make_shared<Channel<Subst>>();
  std::thread worker([&rule, &workset, channel]() {
    auto inputs = rule.getInputs();
    unifyRest(inputs.begin(), inputs.end(), workset, Subst(), *channel);
    channel->close();
  });
  worker.detach();
  return channel;
}

bool Solver::unifyRest(std::vector<Atom>::const_iterator begin,
                       std::vector<Atom>::const_iterator end,
                       WorkingDataset &workset, const Subst &prev,
                       Channel<Subst> &channel, bool wasNewFact) {
  if (begin == end)
    return !wasNewFact || channel.put(prev);
  // проверить все возможные факты для данного атома, если нашли совпадение -
  // проверяем следующие атомы
  auto &curr = *begin++;
  for (const auto &fact : workset.getFacts(curr.getName())) {
    Subst subst = prev;
    if (unify(curr, fact, subst))
      if (!unifyRest(begin, end, workset, subst, channel,
                     wasNewFact || workset.factIsNew(fact)))
        return false;
  }
  return true;
}

bool Solver::unify(const Atom &left, const Atom &right, Subst &subst) {
  if (left.getName() != right.getName())
    return false;
  auto &args1 = left.getArguments();
  auto &args2 = right.getArguments();
  if (args1.size() != args2.size())
    return false;
  for (size_t i = 0; i < args1.size(); ++i) {
    if (!isVar(args1[i]) && !isVar(args2[i])) {
      if (args1[i] != args2[i])
        return false;
    } else if (isVar(args1[i])) {
      if (!subst.insert(args1[i], args2[i]))
        return false;
    } else {
      if (!subst.insert(args2[i], args1[i]))
        return false;
    }
  }
  return true;
}

Atom Solver::applySubst(const Subst &subst, const Atom &atom) {
  auto args = atom.getArguments();
  for (auto &arg : args)
    arg = subst.apply(arg);
  return Atom(atom.getName(), std::move(args));
}
