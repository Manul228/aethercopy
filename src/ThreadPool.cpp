#include "aethercopy/ThreadPool.h"
#include <iostream>

using namespace aethercopy;

ThreadPool::ThreadPool(size_t num_threads) : shutdownRequested_(false), busyThreads_(0)
{
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(ThreadWorker(this));
    }
}

void ThreadPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        shutdownRequested_ = true;
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

ThreadPool::ThreadWorker::ThreadWorker(ThreadPool *p) : tp_(p) {};
ThreadPool::ThreadWorker::~ThreadWorker() = default;

void ThreadPool::ThreadWorker::operator()()
{
    std::unique_lock<std::mutex> lock(tp_->mtx_);

    while (true) {
        // условие пробуждения
        tp_->cv_.wait(lock, [this]() {
            return tp_->shutdownRequested_ or not tp_->queue_.empty();
        });

        if (tp_->shutdownRequested_ and tp_->queue_.empty())
            break;

        if (not tp_->queue_.empty()) {
            auto f = std::move(tp_->queue_.front());
            tp_->queue_.pop();

            lock.unlock();
            tp_->busyThreads_.fetch_add(1);
            try {
                f();
            }
            catch (const std::exception &e) {
                std::clog << "Error when try to execute task: " << e.what() << '\n';
            }
            tp_->busyThreads_.fetch_sub(1);
            tp_->waitCv_.notify_one();
            lock.lock(); // убрать отсюда нельзя
        }
    }
}

void ThreadPool::wait()
{
    std::unique_lock<std::mutex> lock(mtx_);
    waitCv_.wait(lock, [this]() {
        return queue_.empty() and busyThreads_.load() == 0;
    });
}