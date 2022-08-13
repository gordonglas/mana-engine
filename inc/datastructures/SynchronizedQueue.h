#pragma once

#include <functional>
#include <optional>
#include <queue>
#include <utility>
#include "concurrency/Lock.h"

namespace Mana {

template <typename T>
class SynchronizedQueue {
 public:
  SynchronizedQueue() {}
  ~SynchronizedQueue() {}

  void Push(const T& value);
  void Push(T&& value);

  // Returns front without popping it off
  //T& PeekFront();

  // Returns nullopt if empty, else returns front and pops it off.
  std::optional<std::reference_wrapper<T>> Pop();

  //const size_t Size();

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

//template <typename T>
//T& SynchronizedQueue<T>::PeekFront() {
//  ScopedCriticalSection lock(m_lock);
//  return m_queue.front();
//}

//template <typename T>
//T& SynchronizedQueue<T>::Pop() {
//  ScopedCriticalSection lock(m_lock);
//  T& front = m_queue.front();
//  m_queue.pop();
//  return front;
//}

//template <typename T>
//const size_t SynchronizedQueue<T>::Size() {
//  ScopedCriticalSection lock(m_lock);
//  return (size_t)m_queue.size();
//}

// Having PeekFront, Pop, and Size functions
// like above, can cause race conditions.
// So we will only have a Pop function
// that checks for empty, and if empty, returns a "null item".
template <typename T>
std::optional<std::reference_wrapper<T>> SynchronizedQueue<T>::Pop() {
  ScopedCriticalSection lock(m_lock);
  if (m_queue.empty()) {
    return std::nullopt;
  }
  T& front = m_queue.front();
  m_queue.pop();
  return front;
}

}  // namespace Mana
