#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

template <typename T> class Channel {
public:
  Channel(size_t capacity = 1) : m_capacity(capacity) {}

  bool isClosed() const { return m_closed; }
  void close() {
    m_closed = true;
    m_condPut.notify_all();
    m_condGet.notify_all();
  }

  std::pair<T, bool> get() {
    std::unique_lock lock(m_mutex);
    if (m_deque.empty() && !m_closed)
      m_condGet.wait(lock, [this]() { return m_closed || !m_deque.empty(); });
    if (m_deque.empty() && m_closed)
      return {T(), false};
    T object = m_deque.front();
    m_deque.pop_front();
    m_condPut.notify_one();
    return {object, true};
  }

  bool put(T object) {
    std::unique_lock lock(m_mutex);
    if (m_deque.size() == m_capacity)
      m_condPut.wait(
          lock, [this]() { return m_closed || m_deque.size() < m_capacity; });
    if (m_closed)
      return false;
    m_deque.push_back(std::move(object));
    m_condGet.notify_one();
    return true;
  }

private:
  bool m_closed = false;
  std::deque<T> m_deque;
  size_t m_capacity;
  std::mutex m_mutex;
  std::condition_variable m_condGet;
  std::condition_variable m_condPut;
};
