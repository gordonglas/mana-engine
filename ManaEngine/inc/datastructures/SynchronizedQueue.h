#pragma once

#include <atomic>
#include <optional>
#include <queue>
#include <utility>
#include <vector>
#include "concurrency/Mutex.h"

namespace Mana {

// A cross-thread synchronized queue.
template <typename T>
class SynchronizedQueue {
 public:
  SynchronizedQueue() : bEmpty_(true) {}
  ~SynchronizedQueue() {}

  bool Empty_NoLock();
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
  std::atomic<bool> bEmpty_;
  Mutex m_lock;
  std::queue<T> m_queue;
};

template <typename T>
bool SynchronizedQueue<T>::Empty_NoLock() {
  // Purposely not using a lock here, but an atomic<bool> instead.
  // This allows the game-loop thread to avoid locking when it doesn't need to.
  return bEmpty_;
}

template <typename T>
const size_t SynchronizedQueue<T>::Size() {
  ScopedMutex lock(m_lock);
  return (size_t)m_queue.size();
}

template <typename T>
void SynchronizedQueue<T>::Push(const T& value) {
  ScopedMutex lock(m_lock);
  m_queue.push(value);
  bEmpty_ = false;
}

template <typename T>
void SynchronizedQueue<T>::Push(T&& value) {
  ScopedMutex lock(m_lock);
  m_queue.push(std::move(value));
  bEmpty_ = false;
}

template <typename T>
std::optional<T> SynchronizedQueue<T>::PeekFront() {
  ScopedMutex lock(m_lock);
  if (m_queue.empty()) {
    return std::nullopt;
  }
  return m_queue.front();
}

template <typename T>
std::optional<T> SynchronizedQueue<T>::Pop() {
  ScopedMutex lock(m_lock);
  if (m_queue.empty()) {
    return std::nullopt;
  }
  // This can still cause a problem if the object's copy constructor
  // throws an exception. But purposely not using a shared_ptr here
  // for performance reasons.
  // See "Listing 3.5" in book "C++ Concurrency in Action" for details.
  T front = m_queue.front();
  m_queue.pop();

  if (m_queue.empty())
    bEmpty_ = true;

  return front;
}

template <typename T>
std::vector<T> SynchronizedQueue<T>::PopAll() {
  std::vector<T> popped;

  ScopedMutex lock(m_lock);
  while (!m_queue.empty()) {
    T front = m_queue.front();
    m_queue.pop();
    popped.push_back(front);
  }

  bEmpty_ = true;
  return popped;
}

}  // namespace Mana
