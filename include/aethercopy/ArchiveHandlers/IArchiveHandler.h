#ifndef IARCHIVEHANDLER_H
#define IARCHIVEHANDLER_H

#include <string>

namespace aethercopy {
class IArchiveHandler
{
public:
    virtual int64_t getUncompressedArchiveSize(const std::string &archivePath);
    /**
     * @brief extractToDisk
     * @param archivePath Путь к архиву
     * @return путь к распакованному архиву
     * [Документация](https://github.com/libarchive/libarchive/wiki/Examples#user-content-A_Complete_Extractor)
     */
    virtual std::string extractToDisk(const std::string &archivePath, const std::string &targetPath);
};
} // namespace aethercopy

#endif // IARCHIVEHANDLER_H
