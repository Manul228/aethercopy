#include "aethercopy/copiers/IoUringCopier.hpp"
#include <gtest/gtest.h>

TEST(IoRing, CorstructWithValidEntries)
{
    EXPECT_NO_THROW(IoURing{ 64 });
}

TEST(IoRing, ThrowsOnZeroEntries)
{
    // io_uring не принимает 0 слотов
    EXPECT_THROW(IoURing{ 0 }, std::system_error);
}

TEST(IoRing, GetReturnsValidPointer)
{
    IoURing ring{ 64 };
    EXPECT_NE(ring.get(), nullptr);
}

TEST(IoRing, ThrowsWhenSQFull)
{
    IoURing ring{ 4 };

    EXPECT_THROW(
        for (int i = 0; i < 100; ++i) ring.get_sqe();, std::runtime_error
    );
}
