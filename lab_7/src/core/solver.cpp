#include "solver.h"
#include "channel.h"
#include "database.h"
#include "subst.h"
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <thread>

Solver::Solver(std::shared_ptr<Database> database)
    : m_database(std::move(database)) {}

Solver::~Solver() { done(); }

void Solver::solveForward(Atom target) {
  done();
  m_channel = std::make_shared<Channel<Subst>>();
  m_solverThread = std::thread(
      [this, lock = shared_from_this(), target = std::move(target)]() {
        solveForwardThreaded(std::move(target), *m_channel);
        std::cout << "channel closed!\n";
        m_channel->close();
      });
}

void Solver::solveBackward(Atom target) {
  done();
  m_channel = std::make_shared<Channel<Subst>>();
  m_solverThread = std::thread(
      [this, lock = shared_from_this(), target = std::move(target)]() {
        solveBackwardThreaded(std::move(target), *m_channel);
        m_channel->close();
      });
}

std::optional<Subst> Solver::next() {
  if (!m_channel)
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
  for (auto &rule : m_database->getRules()) {
    if (!rule.isFact())
      continue;
    // проверить, находится ли цель среди фактов в базе правил
    Subst subst;
    if (unify(rule.getOutput(), target, subst)) {
      bool wasEmpty = subst.empty();
      if (!output.put(std::move(subst)))
        return;
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
    for (auto &rule : m_database->getRules()) {
      std::cout << "rule: " << rule.toString() << std::endl;
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
      std::cout << "start unifying\n";
      auto [worker, channel] = unifyInputs(rule, workset);
      std::cout << "new iteration!\n";
      while (true) {
        // перебираем все возможные подстановки
        std::cout << "unifying inputs...\n";
        auto [subst, ok] = channel->get();
        std::cout << "got " << subst.toString() << " " << std::boolalpha << ok
                  << std::endl;
        if (!ok)
          break;
        auto newFact = subst.apply(rule.getOutput());
        // проверить, что факт действительно новый
        if (workset.hasFact(newFact))
          continue;
        // проверить, что новый факт удовлетворяет цели
        Subst res;
        if (unify(newFact, target, res)) {
          std::cout << "outputing res = " << res.toString() << std::endl;
          if (!output.put(std::move(res))) {
            std::cout << "closed!\n";
            channel->close();
            return;
          }
          std::cout << "ok!\n";
        }
        workset.addFact(newFact);
        newAdded = true;
      }
      std::cout << "new fact: " << newAdded << std::endl;
      channel->close();
    }
    std::cout << "new fact added: " << newAdded << "\n";
  }
  std::cout << "done!\n";
}

void Solver::solveBackwardThreaded(Atom target, Channel<Subst> &output) {}

std::pair<std::jthread, std::shared_ptr<Channel<Subst>>>
Solver::unifyInputs(const Rule &rule, WorkingDataset &workset) {
  auto channel = std::make_shared<Channel<Subst>>();
  std::jthread worker([&rule, &workset, channel]() {
    auto inputs = rule.getInputs();
    std::cout << "unifying inputs for " << rule.toString() << std::endl;
    unifyRest(inputs.begin(), inputs.end(), workset, Subst(), *channel);
    std::cout << "done unifying!\n";
    channel->close();
  });
  return std::make_pair(std::move(worker), channel);
}

bool Solver::unifyRest(std::vector<Atom>::const_iterator begin,
                       std::vector<Atom>::const_iterator end,
                       WorkingDataset &workset, const Subst &prev,
                       Channel<Subst> &channel, bool wasNewFact) {
  if (begin == end)
    return !wasNewFact || (!channel.isClosed() && channel.put(prev));
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
  for (size_t i = 0; i < args1.size(); ++i)
    if (!unify(args1[i], args2[i], subst))
      return false;
  return true;
}

bool Solver::unify(Variable::ptr left, Variable::ptr right, Subst &subst) {
  if (left->isConst() && right->isConst())
    return left->getValue() == right->getValue();
  if (left->isVariable() && !right->isVariable())
    return subst.insert(left->getValue(), right);
  if (!left->isVariable() && right->isVariable())
    return subst.insert(right->getValue(), left);
  if (left->isVariable() && right->isVariable())
    return left->getValue() == right->getValue() ||
           subst.link(left->getValue(), right->getValue());
  // унификация функциональных символов
  if (left->getValue() != right->getValue())
    return false;
  const auto &args1 = left->getArguments();
  const auto &args2 = right->getArguments();
  if (args1.size() != args2.size())
    return false;
  for (int i = 0; i < args1.size(); ++i)
    if (!unify(args1[i], args2[i], subst))
      return false;
  return true;
}
