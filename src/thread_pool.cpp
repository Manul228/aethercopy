#include "thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads)
    : shutdown_requested_(false)
    , busy_threads_(0)
{
    for (size_t i; i < num_threads; ++i) {
        workers_.emplace_back(ThreadWorker(this));
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        shutdown_requested_ = true;
    }
    cv_.notify_all();

    for (auto &w : workers_) {
        if (w.joinable())
            w.join();
    }
}

template<typename F, typename... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args)
    -> std::future<typename std::invoke_result_t<F, Args...>>
// -> std::future<typename std::invoke_result<F, Args...>>::type
{
    using R = std::invoke_result_t<F, Args...>;
    std::packaged_task<R()> task(std::forward<F>(f), std::forward<Args>(args)...);

    auto fut = task.get_future();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(task);
        cv_.notify_one();
    }

    return fut;
}

ThreadPool::ThreadWorker::ThreadWorker(ThreadPool *p)
    : tp_(p) {};

void ThreadPool::ThreadWorker::operator()()
{
    std::unique_lock<std::mutex> lock(tp_->mtx_);

    while (true) {
        // когда в принципе пора проснуться
        tp_->cv_.wait(lock,
                      [this]() { return tp_->shutdown_requested_ or not tp_->queue_.empty(); });

        if (tp_->shutdown_requested_ and tp_->queue_.empty())
            break;

        if (not tp_->queue_.empty()) {
            auto f = std::move(tp_->queue_.front());
            tp_->queue_.pop();

            // локать дальше не надо
            lock.unlock();
            tp_->busy_threads_.fetch_add(1);
            try {
                f();
            } catch (const std::exception &e) {
                std::clog << "Error when try to execute task: " << e.what() << '\n';
            }
            tp_->busy_threads_.fetch_sub(1);
        }
    }
};