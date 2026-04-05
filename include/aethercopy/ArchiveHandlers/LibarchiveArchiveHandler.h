#ifndef LIBARCHIVEARCHIVEHANDLER_H
#define LIBARCHIVEARCHIVEHANDLER_H

#include "IArchiveHandler.h"

namespace aethercopy {

class LibarchiveArchiveHandler : public IArchiveHandler
{
  public:
    /**
     * Получает размер содержимого архива
     * [Тут примеры обхода
     * архива](https://github.com/libarchive/libarchive/wiki/Examples)
     * @param archivePath Путь к архиву
     */
    // TODO: протестировать корректность для крайних случаев
    // (размер содержимого 0)
    int64_t getUncompressedArchiveSize(const std::string& archivePath) override;
    /**
     * @brief extractToDisk
     * @param archivePath Путь к архиву
     * @return путь к распакованному архиву
     * [Документация](https://github.com/libarchive/libarchive/wiki/Examples#user-content-A_Complete_Extractor)
     */
    bool extractToDisk(const std::string& archivePath,
                       const std::string& targetPath) override;
};

} // namespace aethercopy
#endif // LIBARCHIVEARCHIVEHANDLER_H
