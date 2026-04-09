#include "aethercopy/copiers/IoURingCopier.hpp"
#include <absl/log/log.h>
#include <filesystem>
#include <print>

using namespace aethercopy;

IoURingCopier::IoURingCopier(unsigned ring_entries, size_t chunk_size)
    : ring_(ring_entries)
    , chunk_size_(chunk_size)
    , worker_([this](std::stop_token st) {
        worker_loop(st);
    })
{
}

IoURingCopier::~IoURingCopier()
{
    wait_complete();

    {
        std::lock_guard<std::mutex> lock(ring_mutex_);

        io_uring_sqe* sqe = ring_.get_sqe();

        /*
         * worker_loop надо дать понять, что больше
         * cqe не будет и пора завершать работу
         */
        if (sqe)
        {
            io_uring_prep_nop(sqe);
            sqe->user_data = 0xDEADBEEF;
            ring_.submit();
        }
    }

    worker_.request_stop();
}

void IoURingCopier::wait_complete()
{
    std::unique_lock lock(jobs_mutex_);
    jobs_done_.wait(lock, [this] {
        return active_jobs_.empty();
    });
}

void IoURingCopier::copy(const std::string& src, const std::string& dst)
{
    auto ctx = std::make_unique<JobCtx>();
    ctx->src = src;
    ctx->dst = dst;

    struct stat st{};
    if (stat(src.c_str(), &st) < 0)
    {
        DLOG(ERROR) << std::format("stat {} failed", src);
        return;
    }

    std::filesystem::create_directories(
        std::filesystem::path(dst).parent_path()
    );

    int outfd = open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outfd < 0)
    {
        DLOG(ERROR) << std::format("open {} failed", dst);
        return;
    }

    ctx->file_size  = st.st_size;
    ctx->bytes_left = st.st_size;
    ctx->offset     = 0;
    ctx->buf        = new char[chunk_size_];
    ctx->buf_size   = chunk_size_;
    ctx->outfd      = outfd;

    int infd = open(src.c_str(), O_RDONLY);
    if (infd < 0)
    {
        std::println("open {} failed", src);
        delete[] ctx->buf;
        close(outfd);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(ring_mutex_);

        auto* job         = ctx.get();
        auto  key         = reinterpret_cast<uintptr_t>(job);
        ctx->infd         = infd;
        active_jobs_[key] = std::move(ctx);

        io_uring_sqe* sqe = ring_.get_sqe();
        sqe->user_data    = reinterpret_cast<__u64>(job);
        io_uring_prep_read(sqe, infd, job->buf, job->buf_size, 0);
        ring_.submit();
    }
}

void IoURingCopier::worker_loop(std::stop_token stop)
{
    while (!stop.stop_requested() || !active_jobs_.empty())
    {
        io_uring_cqe* cqe = ring_.wait_cqe();
        if (!cqe)
            continue;

        if (cqe->user_data == 0xDEADBEEF)
        {
            ring_.cqe_seen(cqe);
            break;
        }

        JobCtx* job = reinterpret_cast<JobCtx*>(cqe->user_data);

        if (cqe->res < 0)
        {
            DLOG(ERROR) << std::format(
                "operation failed: {}", strerror(-cqe->res)
            );
            ring_.cqe_seen(cqe);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock{ ring_mutex_ };

            if (job->stage == Stage::Read)
            {
                size_t to_read = (job->bytes_left > (off_t)chunk_size_)
                                     ? chunk_size_
                                     : (size_t)job->bytes_left;

                job->bytes_readed = cqe->res;

                DLOG(INFO) << std::format(
                    "read successful: {} bytes", job->bytes_readed
                );

                job->stage = Stage::Write;

                io_uring_sqe* sqe = ring_.get_sqe();
                sqe->user_data    = reinterpret_cast<__u64>(job);
                io_uring_prep_write(
                    sqe, job->outfd, job->buf, job->bytes_readed, job->offset
                );
                ring_.submit();
            }
            else if (job->stage == Stage::Write)
            {
                DLOG(INFO) << std::format(
                    "write successful: {} bytes", cqe->res
                );

                job->bytes_left -= cqe->res;
                job->offset += cqe->res;

                /*
                 * Если осталось ещё что читать, то добавляем новую задачу
                 * на чтение
                 */
                if (job->bytes_left > 0)
                {
                    job->stage = Stage::Read;

                    size_t to_read = (job->bytes_left > (off_t)chunk_size_)
                                         ? chunk_size_
                                         : (size_t)job->bytes_left;

                    io_uring_sqe* sqe = ring_.get_sqe();
                    sqe->user_data    = reinterpret_cast<__u64>(job);

                    io_uring_prep_read(
                        sqe, job->infd, job->buf, to_read, job->offset
                    );

                    ring_.submit();
                }
                else
                {
                    fsync(job->outfd);
                    close(job->infd);
                    close(job->outfd);

                    std::string src = job->src;
                    std::string dst = job->dst;

                    delete[] job->buf;

                    auto key = reinterpret_cast<uintptr_t>(job);
                    active_jobs_.erase(key);

                    {
                        DLOG(INFO) << std::format(
                            "complete writing {} -> {}", src, dst
                        );
                        std::lock_guard jlock(jobs_mutex_);
                        jobs_done_.notify_one();
                    }
                }
            }
        }

        ring_.cqe_seen(cqe);
    }
}
