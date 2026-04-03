#include "aethercopy/copiers/SyncCopier.h"
#include <filesystem>

void aethercopy::SyncCopier::copy(const std::string &src, const std::string &dst) override
{
    std::filesystem::copy(src, dst);
}