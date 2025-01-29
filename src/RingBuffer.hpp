#pragma once

#include <QDebug>
#include <array>
#include <condition_variable>
#include <fftconv/aligned_vector.hpp>
#include <mutex>

// NOLINTNEXTLINE(*-numbers)
template <typename T, size_t Size = 8> class RingBuffer {
public:
  using ValueType = std::shared_ptr<T>;

  RingBuffer() {
    for (auto &val : buffer) {
      val = std::make_shared<T>();
    }
  };

  template <typename Func> void forEach(const Func &func) {
    for (auto &val : buffer) {
      func(val);
    }
  }

  void quit() {
    std::unique_lock<std::mutex> lock(mutex);
    if (full) {
      tail = (tail + 1) % buffer.size();
    }
    qDebug() << "Quit at tail " << tail;
    buffer[head] = nullptr;
    head = (head + 1) % buffer.size();
    full = head == tail;
    notEmpty.notify_one();
  }

  // Add an element to the buffer. The `produceFunc` should take `T&` and write
  // to it
  template <typename Func> bool produce(const Func &produceFunc) {
    std::unique_lock<std::mutex> lock(mutex);
    if (full) {
      tail = (tail + 1) % buffer.size();
    }
    // qDebug() << "Produce at head" << head;
    produceFunc(buffer[head]);
    head = (head + 1) % buffer.size();
    full = head == tail;
    notEmpty.notify_one();
    return true;
  }

  // Add an element to the buffer. The `produceFunc` should take `T&` and write
  // to it
  template <typename Func> bool produce_nolock(const Func &produceFunc) {
    // std::unique_lock<std::mutex> lock(mutex);
    if (full) {
      tail = (tail + 1) % buffer.size();
    }
    // qDebug() << "Produce at head" << head;
    produceFunc(buffer[head]);
    head = (head + 1) % buffer.size();
    full = head == tail;
    notEmpty.notify_one();
    return true;
  }

  // `consumeFunc` should take `const T&` and read the value
  template <typename Func> void consume(const Func &consumeFunc) {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [this]() { return !empty(); });
    if (empty()) {
      return;
    }
    // qDebug() << "Consume at tail" << tail;
    consumeFunc(buffer[tail]);
    tail = (tail + 1) % buffer.size();
    full = false;
  }

  // `consumeFunc` should take `const T&` and read the value
  template <typename Func> void consume_head(const Func &consumeFunc) {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [this]() { return !empty(); });
    if (empty()) {
      return;
    }
    // Consume at head - 1
    const auto prevHead = (head - 1 + buffer.size()) % buffer.size();
    // qDebug() << "Consume at prevHead" << prevHead;
    consumeFunc(buffer[prevHead]);
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
  std::array<ValueType, Size> buffer;
  size_t head{0};
  size_t tail{0};
  bool full{false};

  mutable std::mutex mutex;
  std::condition_variable notEmpty;
};