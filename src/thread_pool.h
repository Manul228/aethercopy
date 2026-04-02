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
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    template<typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::invoke_result_t<F, Args...>>
    // -> std::future<typename std::invoke_result<F, Args...>>::type
    {
        using R = std::invoke_result_t<F, Args...>;
        std::packaged_task<R()> task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        auto fut = task.get_future();
        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(std::move(task));
            cv_.notify_one();
        }

        return fut;
    }

    void shutdown();

private:
    class ThreadWorker
    {
    public:
        ThreadPool *tp_;
        explicit ThreadWorker(ThreadPool *p);
        ~ThreadWorker();

        void operator()();
    };

    std::vector<std::thread> workers_;
    std::atomic_size_t busy_threads_;
    std::atomic_bool shutdown_requested_;
    std::condition_variable cv_;
    mutable std::mutex mtx_;
    std::queue<std::move_only_function<void()>> queue_;
};

#endif