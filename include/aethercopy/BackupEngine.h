#ifndef ARCHIVE_PROCESSOR_H
#define ARCHIVE_PROCESSOR_H

#include <string>

#include "aethercopy/ArchiveHandlers/IArchiveHandler.h"
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
    BackupEngine(ThreadPool &pool,
                 ICopier &copier,
                 FormatFilter &filter,
                 IArchiveHandler &handler,
                 const std::string &targetBase);
    ~BackupEngine();
    bool processFile(const std::string &path);
    void setTempDir(const std::string &dir);
    void setMaxMemorySize(size_t bytes);

private:
    bool processDirectory(const std::string &dirPath);
    std::string getTargetPath(const std::string &filepath, const std::string &mime);
    bool removeTempDir(const std::string &tempDir);

private:
    std::string tempDir_;
    std::string targetBase_;
    FormatDetector detector_;
    FormatFilter &filter_;
    IArchiveHandler &archiveHandler_;
    ThreadPool &pool_;
    ICopier &copier_;
};

} // namespace aethercopy
#endif // ARCHIVE_PROCESSOR_H
