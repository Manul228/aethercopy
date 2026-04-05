#include "aethercopy/copiers/SyncCopier.h"
#include <filesystem>

using namespace aethercopy;

void SyncCopier::copy(const std::string &src, const std::string &dst)
{
    std::filesystem::path dstPath(dst);
    auto parentDir = dstPath.parent_path();

    if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
        std::filesystem::create_directories(parentDir);
    }

    std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
}