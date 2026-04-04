#include <filesystem>
#include <gtest/gtest.h>

#include "aethercopy/ArchiveHandlers/LibarchiveArchiveHandler.h"
#include "aethercopy/BackupEngine.h"
#include "aethercopy/copiers/SyncCopier.h"

using namespace aethercopy;
namespace fs = std::filesystem;

class BackupEngineTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        std::string testDataDir = TEST_DATA_DIR;
        testSource_ = testDataDir + "Sample1";
        testTarget_ = "/tmp/aethercopy_test_" + std::to_string(std::time(nullptr));
        fs::create_directories(testTarget_);
    }

    void TearDown() override
    {
        fs::remove_all(testTarget_);
    }

    std::string testSource_;
    std::string testTarget_;
};

TEST_F(BackupEngineTest, ProcessDirectoryCopiesFilesToCorrectFolders)
{
    ThreadPool pool(4);
    SyncCopier copier;
    FormatFilter filter;
    filter.includeOnly({}); // всё копируем

    LibarchiveArchiveHandler handler;
    auto engine =
        std::make_shared<BackupEngine>(pool, copier, filter, handler, testTarget_, "/tmp");

    engine->processDirectory(testSource_);
    engine->wait();

    EXPECT_TRUE(fs::exists(testTarget_ + "/images/"));
    EXPECT_TRUE(fs::exists(testTarget_ + "/images/1.jpg"));

    EXPECT_TRUE(fs::exists(testTarget_ + "/documents/"));
    EXPECT_TRUE(fs::exists(testTarget_ + "/documents/document1.pdf"));
}
