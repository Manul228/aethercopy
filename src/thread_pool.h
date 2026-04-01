#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    template<typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::invoke_result_t<F, Args...>>;

private:
    class ThreadWorker
    {
    public:
        ThreadPool *tp_;
        ThreadWorker(ThreadPool *p);
        ~ThreadWorker();

        void operator()();
    };

    std::vector<std::thread> workers_;
    std::atomic_int busy_threads_;
    std::atomic_bool shutdown_requested_;
    std::condition_variable cv_;
    mutable std::mutex mtx_;
    std::queue<std::move_only_function<void()>> queue_;
};

#endif