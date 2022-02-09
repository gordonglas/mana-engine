#pragma once

#include <queue>
#include <utility>
#include "utils/Lock.h"

namespace Mana {

template <typename T>
class SynchronizedQueue {
 public:
  SynchronizedQueue() {}
  ~SynchronizedQueue() {}

  void Push(const T& value);
  void Push(T&& value);

  // returns front without popping it off
  T& PeekFront();

  // returns front and pops it off
  T& Pop();

  const size_t Size();

  SynchronizedQueue(const SynchronizedQueue&) = delete;
  SynchronizedQueue& operator=(const SynchronizedQueue&) = delete;

 private:
  CriticalSection m_lock;
  std::queue<T> m_queue;
};

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
T& SynchronizedQueue<T>::PeekFront() {
  ScopedCriticalSection lock(m_lock);
  return m_queue.front();
}

template <typename T>
T& SynchronizedQueue<T>::Pop() {
  ScopedCriticalSection lock(m_lock);
  T& front = m_queue.front();
  m_queue.pop();
  return front;
}

template <typename T>
const size_t SynchronizedQueue<T>::Size() {
  ScopedCriticalSection lock(m_lock);
  return (size_t)m_queue.size();
}

}  // namespace Mana
