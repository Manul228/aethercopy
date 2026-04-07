#include "aethercopy/copiers/IoUringCopier.hpp"
#include <absl/log/log.h>
#include <filesystem>
#include <gtest/gtest.h>

std::uintmax_t get_file_size(const std::string& path)
{
    try
    {
        return std::filesystem::file_size(path);
    } catch (const std::filesystem::filesystem_error& e)
    {
        DLOG(ERROR) << e.what() << '\n';
        return 0;
    }
}

TEST(IoURingCopierTest, JustRunPlease)
{
    struct io_uring ring;
    off_t           insize;
    int             ret;

    std::string testDataDir = TEST_DATA_DIR;

    std::string srcPath    = testDataDir + "Sample1/document2.pdf";
    std::string targetPath = "/tmp/doc_copy.pdf";

    infd = open(srcPath.c_str(), O_RDONLY);
    ASSERT_GE(infd, 0);

    outfd = open(targetPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ASSERT_GE(outfd, 0);

    ASSERT_EQ(setup_context(QD, &ring), 0);

    ASSERT_EQ(get_file_size(infd, &insize), 0);

    ret = copy_file(&ring, insize);

    fsync(outfd);
    close(infd);
    close(outfd);
    io_uring_queue_exit(&ring);

    auto src_size = get_file_size(srcPath);
    auto dst_size = get_file_size(targetPath);

    std::println(std::cout, "src: {} dst: {}", src_size, dst_size);
    ASSERT_EQ(src_size, dst_size);
    // std::filesystem::remove(targetPath);
}
