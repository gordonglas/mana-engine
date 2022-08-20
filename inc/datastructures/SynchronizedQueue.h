#pragma once

#include <optional>
#include <queue>
#include <utility>
#include <vector>
#include "concurrency/Lock.h"

namespace Mana {

template <typename T>
class SynchronizedQueue {
 public:
  SynchronizedQueue() {}
  ~SynchronizedQueue() {}

  const size_t Size();

  void Push(const T& value);
  void Push(T&& value);

  // Returns front without popping it off
  std::optional<T> PeekFront();

  // Returns nullopt if empty, else returns front and pops it off.
  std::optional<T> Pop();
  // Returns nullopt if empty, else returns all popped objects.
  // This is more efficient than looping around Pop to grab all at once.
  std::vector<T> PopAll();

  SynchronizedQueue(const SynchronizedQueue&) = delete;
  SynchronizedQueue& operator=(const SynchronizedQueue&) = delete;

 private:
  // TODO: HERE!!! try using a mutex instead and compare FPS.
  CriticalSection m_lock;
  std::queue<T> m_queue;
};

template <typename T>
const size_t SynchronizedQueue<T>::Size() {
  ScopedCriticalSection lock(m_lock);
  return (size_t)m_queue.size();
}

template <typename T>
void SynchronizedQueue<T>::Push(const T& value) {
  ScopedCriticalSection lock(m_lock);
  m_queue.push(value);
}

template <typename T>
void SynchronizedQueue<T>::Push(T&& value) {
  ScopedCriticalSection lock(m_lock);
  m_queue.push(std::move(value));
}

template <typename T>
std::optional<T> SynchronizedQueue<T>::PeekFront() {
  ScopedCriticalSection lock(m_lock);
  if (m_queue.empty()) {
    return std::nullopt;
  }
  return m_queue.front();
}

template <typename T>
std::optional<T> SynchronizedQueue<T>::Pop() {
  ScopedCriticalSection lock(m_lock);
  if (m_queue.empty()) {
    return std::nullopt;
  }
  T front = m_queue.front();
  m_queue.pop();
  return front;
}

template <typename T>
std::vector<T> SynchronizedQueue<T>::PopAll() {
  std::vector<T> popped;

  ScopedCriticalSection lock(m_lock);
  while (!m_queue.empty()) {
    T front = m_queue.front();
    m_queue.pop();
    popped.push_back(front);
  }

  return popped;
}

}  // namespace Mana
