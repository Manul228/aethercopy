#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <print>
#include <random>

#include "ScopedTimer.hpp"
#include "aethercopy/copiers/IoURingCopier.hpp"

using namespace aethercopy;

static void createFile(const std::string& path, size_t size)
{
    std::ofstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("Cannot create file: " + path);

    const size_t      chunk = 1024 * 1024;
    std::vector<char> buf(chunk);
    std::mt19937      rng(std::random_device{}());

    for (size_t i = 0; i < size; i += chunk)
    {
        size_t toWrite = std::min(chunk, size - i);
        for (size_t j = 0; j < toWrite; ++j)
            buf[j] = rng() % 256;
        f.write(buf.data(), toWrite);
    }
}

class IoURingCopierTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        tmpDir = "/tmp/aethercopy_test";
        std::filesystem::create_directories(tmpDir);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(tmpDir);
    }

    std::string tmpDir;
};

TEST_F(IoURingCopierTest, AsyncReturnManyFiles)
{
    auto copier = std::make_unique<IoURingCopier>(256, 64 * 1024 * 1024);

    std::string largeSrc  = tmpDir + "/large.bin";
    std::string smallSrc1 = tmpDir + "/small1.txt";
    std::string smallSrc2 = tmpDir + "/small2.txt";

    std::println("Creating test files...");
    createFile(largeSrc, 2UL * 1024 * 1024 * 1024);
    createFile(smallSrc1, 10 * 1024);
    createFile(smallSrc2, 5 * 1024);
    std::println("Files created");

    std::string largeDst  = tmpDir + "/large_copy.bin";
    std::string smallDst1 = tmpDir + "/small1_copy.txt";
    std::string smallDst2 = tmpDir + "/small2_copy.txt";

    // Засекаем время вызова copy()
    ScopedTimer copyTimer;
    copier->copy(largeSrc, largeDst);
    copier->copy(smallSrc1, smallDst1);
    copier->copy(smallSrc2, smallDst2);
    double copyTime = copyTimer.elapsed();

    std::println("All copy() calls completed in {:.2f} ms", copyTime);

    EXPECT_LT(copyTime, 100)
        << "copy() calls should return instantly, but took " << copyTime
        << "ms";

    // Ждём завершения
    ScopedTimer waitTimer;
    copier->wait_complete();
    double waitTime = waitTimer.elapsed();

    std::println(
        "Actual copying took {:.2f} ms ({:.2f} seconds)",
        waitTime,
        waitTime / 1000.0
    );

    // Проверяем размеры
    EXPECT_EQ(
        std::filesystem::file_size(largeSrc),
        std::filesystem::file_size(largeDst)
    );
    EXPECT_EQ(
        std::filesystem::file_size(smallSrc1),
        std::filesystem::file_size(smallDst1)
    );
    EXPECT_EQ(
        std::filesystem::file_size(smallSrc2),
        std::filesystem::file_size(smallDst2)
    );

    // Проверяем содержимое
    std::string cmd = std::string("cmp ") + largeSrc + " " + largeDst;
    EXPECT_EQ(std::system(cmd.c_str()), 0) << "large file content mismatch";

    cmd = std::string("cmp ") + smallSrc1 + " " + smallDst1;
    EXPECT_EQ(std::system(cmd.c_str()), 0) << "small file 1 content mismatch";

    cmd = std::string("cmp ") + smallSrc2 + " " + smallDst2;
    EXPECT_EQ(std::system(cmd.c_str()), 0) << "small file 2 content mismatch";
}

TEST_F(IoURingCopierTest, ManySmallFilesConcurrent)
{
    auto copier = std::make_unique<IoURingCopier>(512, 16 * 1024 * 1024);

    const int numFiles = 50;

    std::vector<std::string> srcFiles;
    std::vector<std::string> dstFiles;

    for (int i = 0; i < numFiles; ++i)
    {
        std::string src = tmpDir + std::format("/src_{}.dat", i);
        std::string dst = tmpDir + std::format("/dst_{}.dat", i);

        createFile(src, 100 * 1024);

        srcFiles.push_back(src);
        dstFiles.push_back(dst);
    }

    // отравляем все copy одновременно
    ScopedTimer sendTimer;
    for (int i = 0; i < numFiles; ++i)
    {
        copier->copy(srcFiles[i], dstFiles[i]);
    }

    double sendTime = sendTimer.elapsed();

    std::println("Sent {} files in {:.2f} ms", numFiles, sendTime);

    EXPECT_LT(sendTime, 10) << "should send many copy() calls instantly";

    ScopedTimer totalTimer;
    copier->wait_complete();
    double totalTime = totalTimer.elapsed();

    std::println(
        "total copy time: {:.2f} ms ({:.2f} seconds)",
        totalTime,
        totalTime / 1000.0
    );

    // Проверяем что все скопировались
    for (int i = 0; i < numFiles; ++i)
    {
        EXPECT_EQ(
            std::filesystem::file_size(srcFiles[i]),
            std::filesystem::file_size(dstFiles[i])
        ) << "File "
          << i << " size mismatch";
    }
}