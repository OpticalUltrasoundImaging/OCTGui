#pragma once

#include <chrono>
#include <string>
#include <utility>

namespace OCT {
/**
Use RAII to time a code block.
Example:
{
  Timeit timeit("Func name");
  func();
}
*/
struct TimeIt {
  using clock = std::chrono::high_resolution_clock;
  explicit TimeIt() : start(clock::now()) {}
  [[nodiscard]] float get_ms() const {
    using namespace std::chrono; // NOLINT(*-namespace)
    const auto elapsed = clock::now() - start;
    const auto nano = duration_cast<nanoseconds>(elapsed).count();
    constexpr float fct_nano2mili = 1.0e-6;
    return static_cast<float>(nano) * fct_nano2mili;
  }
  clock::time_point start;
};

template <typename Func> [[nodiscard]] auto measureTime(Func &&func) -> float {
  TimeIt timeit;
  func();
  return timeit.get_ms();
}

} // namespace OCT
