#include "absl/log/log.h"
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>

#include "aethercopy/BackupEngine.h"
#include "aethercopy/detector.h"
#include "aethercopy/mimeTypes.h"

using namespace aethercopy;

namespace fs = std::filesystem;

BackupEngine::BackupEngine(ThreadPool &pool, ICopier &copier, FormatFilter &filter,
                           IArchiveHandler &handler, const std::string &targetBase,
                           const std::string &tempDir)
    : pool_(pool), copier_(copier), filter_(filter), archiveHandler_(handler),
      targetBase_(targetBase), tempDirBase_(tempDir)
{}

BackupEngine::~BackupEngine()
{
}

std::string BackupEngine::generateUniqueTempDir(const std::string &base)
{
    static std::atomic<uint64_t> counter{ 0 };
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto threadId = std::this_thread::get_id();
    std::hash<std::thread::id> hasher;
    uint64_t id = hasher(threadId);

    uint64_t unique = counter.fetch_add(1);

    return base + "_" + std::to_string(now) + "_" + std::to_string(id) + "_" +
           std::to_string(unique);
}

bool BackupEngine::processFile(const std::string &path)
{
    std::string mime = detector_.detect(path);
    // BUG: распакованные из архива odt файлы обрабатываются как архивы
    // libarchive неправильно распознаёт MIME-типы файлов после распаковки архива.
    // if (mime == "application/octet-stream") {
    //     DLOG(INFO) << "Skipping octet-stream file: " << path;
    //     return true;
    // }
    DLOG(INFO) << "------------------" << '\n';
    DLOG(INFO) << "Current file: " << path << '\n';

    // чтобы класс пережил лямбду
    auto self = shared_from_this();

    if (mimetypes::isArchive(mime)) {
        pool_.enqueue([self, path]() {
            DLOG(INFO) << "we are processing archive in lambda " << path << '\n';
            std::string tempDir = self->generateUniqueTempDir(self->tempDirBase_);
            self->archiveHandler_.extractToDisk(path, tempDir);
            self->processDirectory(tempDir);
            self->removeTempDir(tempDir);
        });
    } else {
        pool_.enqueue([self, path, mime]() {
            if (self->filter_.shouldCopy(mime)) {
                auto targetPath = self->getTargetPath(path, mime);
                DLOG(INFO) << "copying file to " << targetPath << '\n';
                DLOG(INFO) << "mime: " << mime << '\n';
                DLOG(INFO) << "---------------------";
                self->copier_.copy(path, targetPath);
            }
        });
    }
    return true;
}

bool BackupEngine::processDirectory(const std::string &dirPath)
{
    try {
        for (const auto &entry : fs::recursive_directory_iterator(dirPath)) {
            if (entry.is_regular_file())
                processFile(entry.path().string());
        }
        // TODO: добавить лог исходных путей файлов
    } catch (const fs::filesystem_error &e) {
        DLOG(INFO) << "[ERROR] [processDirectory] :" << e.what() << '\n';
    }
    return true;
}

bool BackupEngine::removeTempDir(const std::string &tempDirPath)
{
    const fs::path p = tempDirPath;

    if (not fs::exists(p)) {
        DLOG(INFO) << "temp dir not exist: " << p << '\n';
        return true;
    }

    if (not fs::is_directory(p)) {
        DLOG(INFO) << "path is not a directory: " << p << '\n';
        return false;
    }

    std::error_code ec;
    std::uintmax_t removed = fs::remove_all(p, ec);

    if (ec) {
        DLOG(INFO) << "Error removing directory : " << p << " : " << ec.message() << '\n';
        return false;
    }

    if (removed > 0) {
        DLOG(INFO) << "Removed " << removed << " files/directories " << p << '\n';
    }
    return true;
}

// файл с таким именем уже может существовать
std::string BackupEngine::getUniquePath(const std::string &targetPath)
{
    if (not fs::exists(targetPath)) {
        return targetPath;
    }

    fs::path path(targetPath);
    std::string stem = path.stem().string();
    std::string ext = path.extension().string();

    int counter = 1;
    while (true) {
        std::string newPath = path.parent_path().string() + "/" + stem + "_copy"
                              + std::to_string(counter) + ext;
        if (not fs::exists(newPath)) {
            return newPath;
        }
        counter++;
    }
}

std::string BackupEngine::getTargetPath(const std::string &filepath, const std::string &mime)
{
    std::string filename = std::filesystem::path(filepath).filename().string();
    std::string basePath;

    if (mimetypes::isDocument(mime)) {
        basePath = targetBase_ + "/documents/" + filename;
    }
    else if (mimetypes::isImage(mime)) {
        basePath = targetBase_ + "/images/" + filename;
    }
    else if (mimetypes::isVideo(mime)) {
        basePath = targetBase_ + "/videos/" + filename;
    }
    else if (mimetypes::isAudio(mime)) {
        basePath = targetBase_ + "/audio/" + filename;
    }
    else if (mimetypes::isArchive(mime)) {
        basePath = targetBase_ + "/archives/" + filename;
    }
    else {
        basePath = targetBase_ + "/others/" + filename;
    }
    return getUniquePath(basePath);
}

void BackupEngine::wait()
{
    pool_.wait();
}
