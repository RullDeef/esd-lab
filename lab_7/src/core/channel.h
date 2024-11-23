#pragma once

#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <unistd.h>

template <typename T> class ChannelBuf {
public:
  ChannelBuf(size_t capacity = 1) : m_capacity(capacity) {
    if (capacity == 0)
      throw std::runtime_error("zero capacity for buffered channel");
  };

  bool isClosed() {
    std::unique_lock lock(m_mutex);
    return m_closed;
  }

  void close() {
    std::unique_lock lock(m_mutex);
    m_closed = true;
    m_condPut.notify_all();
    m_condGet.notify_all();
  }

  size_t getCapacity() const { return m_capacity; };

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
  const size_t m_capacity;
  std::mutex m_mutex;
  std::condition_variable m_condGet;
  std::condition_variable m_condPut;
};

// unbuffered channel
template <typename T> class Channel {
public:
  ~Channel() { close(); }

  bool isClosed() {
    std::unique_lock lock(m_mutex);
    return m_closed;
  }

  void close() {
    std::unique_lock lock(m_mutex);
    m_closed = true;
    m_cond.notify_all();
  }

  size_t getCapacity() const { return 0; }

  std::pair<T, bool> get() {
    std::unique_lock lock(m_mutex);
    if (m_closed)
      return {T(), false};
    if (!m_hasValue) {
      m_cond.wait(lock, [this]() { return m_closed || m_hasValue; });
      if (m_closed)
        return {T(), false};
    }
    T object = std::move(m_buffer);
    m_hasValue = false;
    m_cond.notify_all();
    return {object, true};
  }

  bool put(T object) {
    std::unique_lock lock(m_mutex);
    if (m_closed)
      return false;
    if (m_hasValue)
      m_cond.wait(lock, [this]() { return m_closed || !m_hasValue; });
    if (m_closed)
      return false;
    m_buffer = std::move(object);
    m_hasValue = true;
    m_cond.notify_one();
    m_cond.wait(lock, [this]() { return m_closed || !m_hasValue; });
    return !m_closed;
  }

private:
  bool m_closed = false;
  T m_buffer;
  bool m_hasValue = false;
  std::mutex m_mutex;
  std::condition_variable m_cond;
};
