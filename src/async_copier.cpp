#include "aethercopy/async_copier.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace aethercopy {

AsyncFileCopier::AsyncFileCopier(size_t buf_size, int queue_depth)
    : bufSize_(buf_size)
    , queueDepth_(queue_depth)
    , ringInited_(false)
{
    if (io_uring_queue_init(queueDepth_ * 2, &ring_, 0) < 0) {
        throw std::runtime_error("io_uring_queue_init failed");
    }
    ringInited_ = true;

    buffers_.resize(queueDepth_);
    for (int i = 0; i < queueDepth_; ++i) {
        if (posix_memalign((void **) &buffers_[i].data, 4096, bufSize_) != 0) {
            cleanupBuffers();
            io_uring_queue_exit(&ring_);
            throw std::runtime_error("posix_memalign failed for buffer");
        }
    }
}

AsyncFileCopier::~AsyncFileCopier()
{
    cleanupBuffers();
    if (ringInited_) {
        io_uring_queue_exit(&ring_);
    }
}

void AsyncFileCopier::copy_file(const std::string &src, const std::string &dst)
{
    int fd_in = open(src.c_str(), O_RDONLY);
    if (fd_in < 0) {
        throw std::runtime_error(std::string("open source failed: ") + strerror(errno));
    }

    struct stat st{};
    if (fstat(fd_in, &st) < 0) {
        close(fd_in);
        throw std::runtime_error(std::string("fstat source failed: ") + strerror(errno));
    }
    off_t file_size = st.st_size;

    int fd_out = open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        close(fd_in);
        throw std::runtime_error(std::string("open destination failed: ") + strerror(errno));
    }

    for (auto &buf : buffers_)
        buf.in_use = false;

    off_t read_offset = 0;
    off_t write_offset = 0;
    int inflight = 0;

    while ((write_offset < file_size) || (inflight > 0)) {
        while (submitRead(fd_in, file_size, read_offset, inflight)) {
        }

        struct io_uring_cqe *cqe;
        int wait_ret = io_uring_wait_cqe(&ring_, &cqe);
        if (wait_ret < 0) {
            close(fd_in);
            close(fd_out);
            throw std::runtime_error(std::string("io_uring_wait_cqe failed: ")
                                     + strerror(-wait_ret));
        }

        handleCqe(fd_out, write_offset, inflight, cqe);
    }

    close(fd_in);
    close(fd_out);
}

bool AsyncFileCopier::submitRead(int fd_in, off_t file_size, off_t &read_offset, int &inflight)
{
    if (read_offset >= file_size)
        return false;
    if (inflight >= queueDepth_)
        return false;

    int idx = findFreeBuffer();
    if (idx < 0)
        return false;

    size_t to_read = (file_size - read_offset > bufSize_) ? bufSize_
                                                          : (size_t) (file_size - read_offset);

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe)
        throw std::runtime_error("io_uring_get_sqe failed");

    buffers_[idx].in_use = true;
    buffers_[idx].offset = read_offset;
    buffers_[idx].length = to_read;

    io_uring_prep_read(sqe, fd_in, buffers_[idx].data, to_read, read_offset);
    sqe->user_data = (unsigned long) idx;

    int ret = io_uring_submit(&ring_);
    if (ret < 0) {
        throw std::runtime_error(std::string("io_uring_submit failed: ") + strerror(-ret));
    }

    read_offset += to_read;
    inflight++;
    return true;
}

bool AsyncFileCopier::submitWrite(int fd_out, int idx, size_t bytes)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe)
        throw std::runtime_error("io_uring_get_sqe failed");

    io_uring_prep_write(sqe, fd_out, buffers_[idx].data, bytes, buffers_[idx].offset);
    sqe->user_data = (unsigned long) (idx + queueDepth_);

    int ret = io_uring_submit(&ring_);
    if (ret < 0) {
        throw std::runtime_error(std::string("io_uring_submit failed: ") + strerror(-ret));
    }
    return true;
}

void AsyncFileCopier::handleCqe(int fd_out,
                                off_t &write_offset,
                                int &inflight,
                                struct io_uring_cqe *cqe)
{
    unsigned int idx = (unsigned int) cqe->user_data;

    if (cqe->res < 0) {
        io_uring_cqe_seen(&ring_, cqe);
        throw std::runtime_error(std::string("IO operation failed: ") + strerror(-cqe->res));
    }

    if (idx < (unsigned) queueDepth_) {
        size_t bytes = (size_t) cqe->res;
        if (bytes == 0) {
            buffers_[idx].in_use = false;
            inflight--;
            io_uring_cqe_seen(&ring_, cqe);
            return;
        }
        submitWrite(fd_out, idx, bytes);
        io_uring_cqe_seen(&ring_, cqe);
    } else {
        unsigned int write_idx = idx - queueDepth_;
        buffers_[write_idx].in_use = false;
        inflight--;
        write_offset += cqe->res;
        io_uring_cqe_seen(&ring_, cqe);
    }
}

int AsyncFileCopier::findFreeBuffer()
{
    for (int i = 0; i < queueDepth_; ++i) {
        if (!buffers_[i].in_use)
            return i;
    }
    return -1;
}

void AsyncFileCopier::cleanupBuffers()
{
    for (auto &buf : buffers_) {
        free(buf.data);
        buf.data = nullptr;
    }
    buffers_.clear();
}
} // namespace aethercopy