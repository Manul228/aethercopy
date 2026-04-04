#include "aethercopy/ArchiveHandlers/LibarchiveArchiveHandler.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace aethercopy;

TEST(LibarchiveArchiveHandlerTest, ShouldCorrectlyCountContentSize)
{
    LibarchiveArchiveHandler handler;

    std::string testDataDir = TEST_DATA_DIR;
    int64_t contentSize = handler.getUncompressedArchiveSize(testDataDir + "Sample1/Archive.zip");
    ASSERT_EQ(contentSize, 1365213);
}

TEST(LibarchiveHandlerTest, ShouldExtractCorrectly)
{
    LibarchiveArchiveHandler handler;

    std::string testDataDir = TEST_DATA_DIR;
    handler.extractToDisk(testDataDir + "Sample1/Archive.zip",
                          "/tmp/aethercopy_libarchivehandlertest/");
    ASSERT_TRUE(std::filesystem::exists("/tmp/aethercopy_libarchivehandlertest/document2.pdf"));
}