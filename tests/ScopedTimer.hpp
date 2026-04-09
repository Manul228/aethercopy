#pragma once

#include <chrono>

// Класс ScopedTimer для измерения времени в области видимости
class ScopedTimer
{
  public:
    using clock = std::chrono::high_resolution_clock;
    using duration_t =
        std::chrono::duration<double, std::milli>; // миллисекунды

    ScopedTimer()
        : start_time(clock::now())
    {
    }

    double elapsed() const
    {
        return duration_t(clock::now() - start_time).count();
    }

  private:
    clock::time_point start_time;
};