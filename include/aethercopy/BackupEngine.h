#ifndef ARCHIVE_PROCESSOR_H
#define ARCHIVE_PROCESSOR_H

#include <memory>
#include <string>

#include "aethercopy/FormatDetector.hpp"
#include "aethercopy/FormatFilter.hpp"
#include "aethercopy/ThreadPool.h"

namespace aethercopy {

class IArchiveHandler;
class ICopier;

/**
 * @brief BackupEngine обрабатывает архивы, если они попадутся
 * Наследуем от std::enable_shared_from_this<BackupEngine>
 * потому что он кидает лямбды в пул потоков, и нужно чтобы он
 * до этого не сдох
 */

class BackupEngine : public std::enable_shared_from_this<BackupEngine>
{
  public:
    // BackupEngine(ThreadPool &pool, ICopier &copier, FormatFilter &filter,
    // IArchiveHandler &handler,
    //              const std::string &targetBase, const std::string &tempDir);

    BackupEngine(std::shared_ptr<ThreadPool> pool,
                 std::shared_ptr<ICopier> copier,
                 std::shared_ptr<IArchiveHandler> archiveHandler,
                 FormatFilter& filter,
                 const std::string& targetBase,
                 const std::string& tempDirBase);

    BackupEngine(const BackupEngine&) = delete;
    BackupEngine& operator=(const BackupEngine&) = delete;

    BackupEngine(BackupEngine&&) = delete;
    BackupEngine& operator=(BackupEngine&&) = delete;

    ~BackupEngine() {};

    bool processDirectory(const std::string& dirPath);
    bool processFile(const std::string& path);
    void wait();

  private:
    /**
     * @brief generateUniqueTempDir Позволяет надёжно получить имя временной
     * директории без этого другой поток может получить ту же временную
     * директорию и удалить её до того как её обработает другой поток, либо
     * подмешает туда файлы
     * @param base База для временных директорий. Обычто tempDirBase_
     * @return
     */
    std::string generateUniqueTempDir(const std::string& base);
    std::string getUniquePath(const std::string& targetPath);
    std::string getTargetPath(const std::string& filepath,
                              const std::string& mime);
    bool removeTempDir(const std::string& tempDir);

  private:
    std::string tempDirBase_;
    std::string targetBase_;
    FormatDetector detector_;
    FormatFilter& filter_;
    std::shared_ptr<IArchiveHandler> archiveHandler_;
    std::shared_ptr<ThreadPool> pool_;
    std::shared_ptr<ICopier> copier_;
};

} // namespace aethercopy
#endif // ARCHIVE_PROCESSOR_H
