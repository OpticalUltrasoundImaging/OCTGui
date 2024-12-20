#pragma once

#include <array>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <vector>

template <typename T, size_t Size = 6> class RingBuffer {
public:
  explicit RingBuffer(size_t size) : buffer(size) {}

  // Add an element to the buffer
  bool push(const T &item) {
    std::unique_lock<std::mutex> lock(mutex);
    if (full) {
      tail = (tail + 1) % buffer.size();
    }
    buffer[head] = item;
    head = (head + 1) % buffer.size();
    full = head == tail;
    notEmpty.notify_one();
    return true;
  }

  // Add an element to the buffer. The `produceFunc` should take `T&` and write
  // to it
  template <typename Func> bool produce(const Func &produceFunc) {
    std::unique_lock<std::mutex> lock(mutex);
    if (full) {
      tail = (tail + 1) % buffer.size();
    }
    produceFunc(buffer[head]);
    head = (head + 1) % buffer.size();
    full = head == tail;
    notEmpty.notify_one();
    return true;
  }

  std::optional<T &> pop() {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [this]() { return !empty(); });
    if (empty()) {
      return std::nullopt;
    }
    T &item = buffer[tail];
    tail = (tail + 1) % buffer.size();
    full = false;
    return item;
  }

  // `consumeFunc` should take `const T&` and read the value
  template <typename Func> void consume(const Func &consumeFunc) {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [this]() { return !empty(); });
    if (empty()) {
      return;
    }
    consumeFunc(buffer[tail]);
    tail = (tail + 1) % buffer.size();
    full = false;
  }

  bool empty() const { return (!full && (head == tail)); }
  bool isFull() const { return full; }

  size_t size() const {
    if (full) {
      return buffer.size();
    }
    if (head >= tail) {
      return head - tail;
    }
    return buffer.size() + head - tail;
  }

private:
  std::array<T, Size> buffer;
  size_t head{0};
  size_t tail{0};
  bool full{false};

  mutable std::mutex mutex;
  std::condition_variable notEmpty;
};

template <typename T> using RingBufferOfVec = RingBuffer<std::vector<T>>;