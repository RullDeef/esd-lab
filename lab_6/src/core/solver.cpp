#include "solver.h"
#include "channel.h"
#include "database.h"
#include "subst.h"
#include <optional>

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

void Solver::solveBackward(Atom target) { return; }

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
  for (auto &fact : m_database.getFacts()) {
    // проверить, находится ли цель среди фактов в базе правил
    Subst subst;
    if (unify(fact, target, subst)) {
      bool wasEmpty = subst.empty();
      output.put(std::move(subst));
      // завершить поиск если была сгенерирована пустая подстановка
      if (wasEmpty)
        return;
    }
    workset.addFact(fact);
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
