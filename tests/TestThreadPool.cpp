#include "aethercopy/ThreadPool.h"
#include <gtest/gtest.h>
#include <iostream>

#include "ScopedTimer.hpp"

using namespace std::chrono_literals;
using namespace aethercopy;

TEST(ThreadPoolTest, SingleTaskWithSleep)
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

TEST(ThreadPoolTest, MultipleTasksLessThanThreads)
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

TEST(ThreadPoolTest, MultipleTasksMoreThanThreads)
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

    /*
     * Ожидаем, что задачи выполняются примерно
     * за 2 периода по 150 мс (6 задач / 3 потока = 2 партии)
     */
    EXPECT_GE(duration_ms, 300);
    EXPECT_LT(duration_ms, 450);
}

TEST(ThreadPoolTest, TasksRunInParallel)
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
}

TEST(ThreadPoolTest, EmptyTasksBenchmark)
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
}

TEST(ThreadPoolTest, WaitTest)
{
    const int thread_count = 4;
    ThreadPool pool(thread_count);

    const int task_count = 8;

    std::atomic_int completed{0};
    for (int i = 0; i < task_count; ++i) {
        pool.enqueue([&]() {
            std::this_thread::sleep_for(100ms);
            ++completed;
        });
    }

    pool.wait();
    EXPECT_EQ(completed.load(), task_count);
}

TEST(ThreadPoolTest, WaitBlocksUntilCompletion)
{
    ThreadPool pool(2);
    std::atomic<bool> finished{false};

    pool.enqueue([&]() {
        std::this_thread::sleep_for(200ms);
        finished = true;
    });

    pool.wait();

    EXPECT_TRUE(finished); // wait дождался выполнения
}
