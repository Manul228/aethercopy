#ifndef ARCHIVE_PROCESSOR_H
#define ARCHIVE_PROCESSOR_H

#include <string>

#include "async_copier.h"

#include "detector.h"
#include "thread_pool.h"

namespace aethercopy {

/**
 * @brief The ArchiveProcessor обрабатывает архивы, если они попадутся
 * Наследуем от std::enable_shared_from_this<ArchiveProcessor>
 * потому что он кидает лямбды в пул потоков, и нужно чтобы он
 * до этого не сдох
 */

class ArchiveProcessor : public std::enable_shared_from_this<ArchiveProcessor>
{
public:
    ArchiveProcessor(ThreadPool &pool, AsyncFileCopier &copier);
    ~ArchiveProcessor();
    void processFile(const std::string &path);
    void setTempDir(const std::string &dir);
    void setMaxMemorySize(size_t bytes);

private:
    bool isSmallArchive(const std::string &path);
    std::string extractToDisk(const std::string &archivePath);
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
    // TODO: нужно контролировать общий расход
    // памяти и задать осмысленное значение по-умолчанию
    size_t maxMemorySize_;
    FormatDetector detector_;
    int64_t cutoffSize_;
    ThreadPool &pool_;
    AsyncFileCopier &copier_;
};

} // namespace aethercopy
#endif // ARCHIVE_PROCESSOR_H
