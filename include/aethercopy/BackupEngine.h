#ifndef ARCHIVE_PROCESSOR_H
#define ARCHIVE_PROCESSOR_H

#include <string>

#include "aethercopy/copiers/ICopier.h"

#include "aethercopy/filter.h"
#include "detector.h"
#include "thread_pool.h"

namespace aethercopy {

/**
 * @brief BackupEngine обрабатывает архивы, если они попадутся
 * Наследуем от std::enable_shared_from_this<BackupEngine>
 * потому что он кидает лямбды в пул потоков, и нужно чтобы он
 * до этого не сдох
 */

class BackupEngine : public std::enable_shared_from_this<BackupEngine>
{
public:
    BackupEngine(ThreadPool &pool, ICopier &copier, const std::string &targetBase);
    ~BackupEngine();
    void processFile(const std::string &path);
    void setTempDir(const std::string &dir);
    void setMaxMemorySize(size_t bytes);

private:
    bool isSmallArchive(const std::string &path);
    /**
     * @brief extractToDisk
     * @param archivePath Путь к архиву
     * @return путь к распакованному архиву
     * [Документация](https://github.com/libarchive/libarchive/wiki/Examples#user-content-A_Complete_Extractor)
     */
    std::string extractToDisk(const std::string &archivePath);

    void processDirectory(const std::string &dirPath);
    std::string getTargetPath(const std::string &filepath, const std::string &mime);
    void removeTempDir(const std::string &tempDir);

    /**
     * Получает размер содержимого архива
     * [Тут примеры обхода архива](https://github.com/libarchive/libarchive/wiki/Examples)
     * @param archivePath Путь к архиву
     */
    // TODO: протестировать корректность для крайних случаев
    // (размер содержимого 0)
    int64_t getUncompressedArchiveSize(const std::string &archivePath);

private:
    std::string tempDir_;
    std::string targetBase_;
    // TODO: нужно контролировать общий расход
    // памяти и задать осмысленное значение по-умолчанию
    size_t maxMemorySize_;
    FormatDetector detector_;
    FormatFilter filter_;
    int64_t cutoffSize_;
    ThreadPool &pool_;
    ICopier &copier_;
};

} // namespace aethercopy
#endif // ARCHIVE_PROCESSOR_H
