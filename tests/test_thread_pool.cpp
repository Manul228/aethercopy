#include "thread_pool.h"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>

using namespace std::chrono_literals;

// Класс ScopedTimer для измерения времени в области видимости
class ScopedTimer
{
public:
    using clock = std::chrono::high_resolution_clock;
    using duration_t = std::chrono::duration<double, std::milli>; // миллисекунды

    ScopedTimer()
        : start_time(clock::now())
    {}

    double elapsed() const { return duration_t(clock::now() - start_time).count(); }

private:
    clock::time_point start_time;
};

class ThreadPoolTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ThreadPoolTest, SingleTaskWithSleep)
{
    ThreadPool pool(2);

    ScopedTimer timer;

    auto fut = pool.enqueue([]() {
        std::this_thread::sleep_for(100ms);
        return 42;
    });

    int result = fut.get();
    double duration_ms = timer.elapsed();

    EXPECT_EQ(result, 42);
    EXPECT_GE(duration_ms, 100); // Задержка минимум 100 мс
}

TEST_F(ThreadPoolTest, MultipleTasksLessThanThreads)
{
    const int thread_count = 4;
    ThreadPool pool(thread_count);

    const int task_count = 2; // меньше потоков
    std::vector<std::future<int>> futures;

    ScopedTimer timer;

    for (int i = 0; i < task_count; ++i) {
        futures.push_back(pool.enqueue([i]() {
            std::this_thread::sleep_for(200ms);
            return i;
        }));
    }

    for (auto &f : futures) {
        f.get();
    }

    double duration_ms = timer.elapsed();

    // Если задачи выполняются параллельно, время будет около 200 мс, а не 400
    EXPECT_LT(duration_ms, 300);
}

TEST_F(ThreadPoolTest, MultipleTasksMoreThanThreads)
{
    const int thread_count = 3;
    ThreadPool pool(thread_count);

    const int task_count = 6; // больше потоков
    std::vector<std::future<int>> futures;

    ScopedTimer timer;

    for (int i = 0; i < task_count; ++i) {
        futures.push_back(pool.enqueue([i]() {
            std::this_thread::sleep_for(150ms);
            return i;
        }));
    }

    for (auto &f : futures) {
        f.get();
    }

    double duration_ms = timer.elapsed();

    // Ожидаем, что задачи выполняются примерно за 2 периода по 150 мс (6 задач / 3 потока = 2 "партии")
    EXPECT_GE(duration_ms, 300);
    EXPECT_LT(duration_ms, 450);
}

TEST_F(ThreadPoolTest, TasksRunInParallel)
{
    const int thread_count = 4;
    ThreadPool pool(thread_count);

    std::atomic<int> running_tasks{0};
    std::atomic<int> max_concurrent{0};

    const int task_count = 8;
    std::vector<std::future<void>> futures;

    ScopedTimer timer;

    for (int i = 0; i < task_count; ++i) {
        futures.push_back(pool.enqueue([&]() {
            running_tasks.fetch_add(1, std::memory_order_relaxed);
            int current = running_tasks.load(std::memory_order_relaxed);
            int prev_max = max_concurrent.load(std::memory_order_relaxed);
            while (current > prev_max && !max_concurrent.compare_exchange_weak(prev_max, current)) {
                // пытаемся обновить максимум
            }
            std::this_thread::sleep_for(100ms);
            running_tasks.fetch_sub(1, std::memory_order_relaxed);
        }));
    }

    for (auto &f : futures) {
        f.get();
    }

    double duration_ms = timer.elapsed();

    EXPECT_GE(max_concurrent.load(), thread_count);
    // Можно при желании проверить, что общее время не слишком большое
    EXPECT_LT(duration_ms, 1000);
}

TEST_F(ThreadPoolTest, EmptyTasksBenchmark)
{
    const int task_count = 1000;
    ThreadPool pool(4); // Можно указать нужное количество потоков

    std::vector<std::future<void>> futures;

    ScopedTimer timer;

    for (int i = 0; i < task_count; ++i) {
        futures.push_back(pool.enqueue([]() {
            // Пустая задача — ничего не делает
        }));
    }

    for (auto &f : futures) {
        f.get();
    }

    double total_ms = timer.elapsed();
    double avg_per_task = total_ms / task_count;

    std::cout << "Total time for " << task_count << " empty tasks: " << total_ms << " ms\n";
    std::cout << "Average time per task: " << avg_per_task << " ms\n";

    // Можно добавить проверку, например, что среднее время не слишком большое
    // EXPECT_LT(avg_per_task, 1.0); // например, менее 1 мс на задачу
}
