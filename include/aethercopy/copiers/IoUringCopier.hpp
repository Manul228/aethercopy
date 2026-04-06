#ifndef IOURINGCOPIER_HPP
#define IOURINGCOPIER_HPP

#include <liburing.h>
#include <system_error>

class IoURing
{
  public:
    explicit IoURing(unsigned entires)
    {
        // ядро само будет следить за SQ
        // и не надо будет вручную вызывать io_uring_submit()
        io_uring_params params{};
        params.flags = IORING_SETUP_SQPOLL;

        int ret = io_uring_queue_init_params(entires, &ring_, &params);

        if (ret < 0)
        {
            throw std::system_error(
                -ret, std::system_category(), "io_uring_queue_init_params"
            );
        }
    }

    ~IoURing()
    {
        io_uring_queue_exit(&ring_);
    }

    IoURing(const IoURing&)            = delete;
    IoURing& operator=(const IoURing&) = delete;

    io_uring* get() noexcept
    {
        return &ring_;
    }

    // если нет свободных слотов, то вернёт nullptr
    io_uring_sqe* get_sqe()
    {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
        if (!sqe)
        {
            throw std::runtime_error("SQ is full");
        }
        return sqe;
    }

    void submit()
    {
        io_uring_submit(&ring_);
    }

  private:
    struct io_uring ring_{};
};

#endif // IOURINGCOPIER_HPP
