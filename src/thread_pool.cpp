#include "thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads)
    : shutdown_requested_(false)
    , busy_threads_(0)
{
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(ThreadWorker(this));
    }
}

void ThreadPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        shutdown_requested_ = true;
    }
    cv_.notify_all();
}

ThreadPool::~ThreadPool()
{
    shutdown();

    for (auto &w : workers_) {
        if (w.joinable())
            w.join();
    }
}

ThreadPool::ThreadWorker::ThreadWorker(ThreadPool *p)
    : tp_(p) {};

ThreadPool::ThreadWorker::~ThreadWorker() = default;

void ThreadPool::ThreadWorker::operator()()
{
    std::unique_lock<std::mutex> lock(tp_->mtx_);

    while (true) {
        // когда в принципе пора проснуться
        // берёт блокировку после предыдущего unlock
        tp_->cv_.wait(lock,
                      [this]() { return tp_->shutdown_requested_ or not tp_->queue_.empty(); });

        if (tp_->shutdown_requested_ and tp_->queue_.empty())
            break;

        if (not tp_->queue_.empty()) {
            auto f = std::move(tp_->queue_.front());
            tp_->queue_.pop();

            lock.unlock();
            tp_->busy_threads_.fetch_add(1);
            try {
                f();
            } catch (const std::exception &e) {
                std::clog << "Error when try to execute task: " << e.what() << '\n';
            }
            tp_->busy_threads_.fetch_sub(1);
            lock.lock();
        }
    }
};