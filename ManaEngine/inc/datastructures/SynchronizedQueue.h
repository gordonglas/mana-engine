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
  SynchronizedQueue() : bEmpty_(true) {};
  virtual ~SynchronizedQueue() = default;

  SynchronizedQueue(const SynchronizedQueue&) = delete;
  SynchronizedQueue& operator=(const SynchronizedQueue&) = delete;

  bool Empty_NoLock();
  const size_t Size();

  void Push(const T& value);
  void Push(T&& value);

  // Returns front without popping it off
  std::optional<T> PeekFront();

  // Returns nullopt if empty, else returns front and pops it off.
  std::optional<T> Pop();
  // Returns all popped objects.
  // This is more efficient than looping around Pop to grab all at once.
  void PopAll(std::vector<T>& popped);

 private:
  std::atomic<bool> bEmpty_;
  Mutex lock_;
  std::queue<T> queue_;
};

template <typename T>
bool SynchronizedQueue<T>::Empty_NoLock() {
  // Purposely not using a lock here, but an atomic<bool> instead.
  // This allows the game-loop thread to avoid locking when it doesn't need to.
  //return bEmpty_;
  return bEmpty_.load(std::memory_order_acquire);
}

template <typename T>
const size_t SynchronizedQueue<T>::Size() {
  ScopedMutex lock(lock_);
  return (size_t)queue_.size();
}

template <typename T>
void SynchronizedQueue<T>::Push(const T& value) {
  ScopedMutex lock(lock_);
  queue_.push(value);
  //bEmpty_ = false;
  bEmpty_.store(false, std::memory_order_release);
}

template <typename T>
void SynchronizedQueue<T>::Push(T&& value) {
  ScopedMutex lock(lock_);
  queue_.push(std::move(value));
  //bEmpty_ = false;
  bEmpty_.store(false, std::memory_order_release);
}

template <typename T>
std::optional<T> SynchronizedQueue<T>::PeekFront() {
  ScopedMutex lock(lock_);
  if (queue_.empty()) {
    return std::nullopt;
  }
  return queue_.front();
}

template <typename T>
std::optional<T> SynchronizedQueue<T>::Pop() {
  ScopedMutex lock(lock_);
  if (queue_.empty()) {
    return std::nullopt;
  }
  // This can still cause a problem if the object's copy constructor
  // throws an exception. But purposely not using a shared_ptr here
  // for performance reasons.
  // See "Listing 3.5" in book "C++ Concurrency in Action" for details.
  T front = queue_.front();
  queue_.pop();

  if (queue_.empty()) {
    //bEmpty_ = true;
    bEmpty_.store(true, std::memory_order_release);
  }

  return front;
}

template <typename T>
void SynchronizedQueue<T>::PopAll(std::vector<T>& popped) {
  popped.clear();

  ScopedMutex lock(lock_);
  while (!queue_.empty()) {
    T front = queue_.front();
    queue_.pop();
    popped.push_back(front);
  }

  //bEmpty_ = true;
  bEmpty_.store(true, std::memory_order_release);
}

}  // namespace Mana
