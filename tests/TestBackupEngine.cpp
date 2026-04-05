#include <gtest/gtest.h>
#include <filesystem>
#include "absl/log/log.h"

#include "aethercopy/ArchiveHandlers/LibarchiveArchiveHandler.h"
#include "aethercopy/BackupEngine.h"
#include "aethercopy/copiers/SyncCopier.h"

using namespace aethercopy;
namespace fs = std::filesystem;

class BackupEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::string testDataDir = TEST_DATA_DIR;
    testSource_ = testDataDir + "Sample1";
    testTarget_ =
        "/tmp/aethercopy_" +
        std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
    fs::create_directories(testTarget_);
  }

  void TearDown() override { fs::remove_all(testTarget_); }

  std::string testSource_;
  std::string testTarget_;
};

TEST_F(BackupEngineTest, ProcessDirectoryCopiesFilesToCorrectFolders) {
  auto pool = std::make_shared<ThreadPool>(4);
  auto copier = std::make_shared<SyncCopier>();
  auto archiveHandler = std::make_shared<LibarchiveArchiveHandler>();
  FormatFilter filter;
  filter.includeOnly({});  // всё копируем

  DLOG(INFO) << "testTarget_ : " << testTarget_ << '\n';
  ASSERT_TRUE(std::filesystem::exists(testTarget_));

  auto engine = std::make_shared<BackupEngine>(pool, copier, archiveHandler,
                                               filter, testTarget_, "/tmp");
  engine->processDirectory(testSource_);
  engine->wait();

  EXPECT_TRUE(fs::exists(testTarget_ + "/images/"));
  EXPECT_TRUE(fs::exists(testTarget_ + "/images/4.jpg"));

  EXPECT_TRUE(fs::exists(testTarget_ + "/documents/"));
  EXPECT_TRUE(fs::exists(testTarget_ + "/documents/document1.pdf"));
}
