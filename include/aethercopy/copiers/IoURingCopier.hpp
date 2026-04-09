#pragma once

#include <condition_variable>
#include <cstring>
#include <fcntl.h>
#include <liburing.h>
#include <sys/stat.h>

#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <unordered_map>

#include "ICopier.h"

namespace aethercopy {

enum Stage : uint8_t
{
    Read,
    Write
};

struct JobCtx
{
    std::string src;
    std::string dst;

    char*  buf       = nullptr;
    size_t buf_size  = 0;
    off_t  file_size = 0;

    int infd  = -1;
    int outfd = -1;

    int bytes_readed = 0;

    off_t bytes_left = 0;
    off_t offset;

    Stage stage = Read;
};

class IoRing
{
  public:
    explicit IoRing(unsigned entries)
    {
        if (io_uring_queue_init(entries, &ring_, 0) < 0)
            throw std::runtime_error("io_uring_queue_init failed");
    }

    ~IoRing()
    {
        io_uring_queue_exit(&ring_);
    }

    io_uring_sqe* get_sqe()
    {
        return io_uring_get_sqe(&ring_);
    }

    void submit()
    {
        io_uring_submit(&ring_);
    }

    io_uring_cqe* wait_cqe()
    {
        io_uring_cqe* cqe;
        if (io_uring_wait_cqe(&ring_, &cqe) < 0)
            return nullptr;
        return cqe;
    }

    void cqe_seen(io_uring_cqe* cqe)
    {
        io_uring_cqe_seen(&ring_, cqe);
    }

  private:
    io_uring ring_{};
};

/**
 * @brief The IoURingCopier Асинхронный, неблокирующий вызывающий поток
 * копировщик.
 * TODO:
 * - Защититься от исчерпания SQ. Вряд ли это произойдёт когда-либо, но всё же
 *
 * - Когда репы линуксов дозреют до версии io_uring 2.16+ перейти на
 * io_uring_prep_copy_file_range. Проблема с исчерпанием SQ уйдёт сама собой,
 * поэтому её мы и не трогаем пока
 *
 * - Внедрить SQPOLL и прямые дескрипторы
 */

class IoURingCopier : public ICopier
{
  public:
    explicit IoURingCopier(
        unsigned ring_entries = 16,
        size_t   chunk_size   = 16 * 1024
    );
    ~IoURingCopier();

    void wait_complete();
    void copy(const std::string& src, const std::string& dst);

  private:
    IoRing       ring_;
    std::jthread worker_;
    std::mutex   ring_mutex_;

    std::mutex              jobs_mutex_;
    std::condition_variable jobs_done_;

    const size_t chunk_size_;

    std::unordered_map<uintptr_t, std::unique_ptr<JobCtx>> active_jobs_;

    void worker_loop(std::stop_token stop);
};

}