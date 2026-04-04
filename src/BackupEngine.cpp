#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <iostream>

#include "aethercopy/BackupEngine.h"
#include "aethercopy/detector.h"
#include "aethercopy/mime_types.h"

namespace aethercopy {

namespace fs = std::filesystem;

BackupEngine::BackupEngine(ThreadPool &pool,
                           ICopier &copier,
                           FormatFilter &filter,
                           IArchiveHandler &handler,
                           const std::string &targetBase)
    : pool_(pool)
    , copier_(copier)
    , filter_(filter)
    , archiveHandler_(handler)
    , targetBase_(targetBase)
{}

bool BackupEngine::processFile(const std::string &path)
{
    std::string mime = detector_.detect(path);
    if (not filter_.shouldCopy(mime))
        return true;

    auto self = shared_from_this();

    if (mimetypes::isArchive(mime)) {
        // чтобы класс пережил лямбду
        pool_.enqueue([self, path]() {
            std::string tempDir = self->archiveHandler_.extractToDisk(path, self->tempDir_);
            self->processDirectory(tempDir);
            self->removeTempDir(tempDir);
        });
    } else {
        pool_.enqueue([self, path, &mime]() {
            //
            self->copier_.copy(path, self->getTargetPath(path, mime));
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
    } catch (const fs::filesystem_error &e) {
        std::clog << "[ERROR] [processDirectory] :" << e.what() << '\n';
    }
    return true;
}

bool BackupEngine::removeTempDir(const std::string &tempDirPath)
{
    const fs::path p = tempDirPath;

    if (not fs::exists(p)) {
        std::clog << "temp dir not exist: " << p << '\n';
        return true;
    }

    if (not fs::is_directory(p)) {
        std::clog << "path is not a directory: " << p << '\n';
        return false;
    }

    std::error_code ec;
    std::uintmax_t removed = fs::remove_all(p, ec);

    if (ec) {
        std::clog << "Error removing directory : " << p << " : " << ec.message() << '\n';
        return false;
    }

    if (removed > 0) {
        std::clog << "Removed " << removed << " files/directories " << p << '\n';
    }
    return true;
}

std::string BackupEngine::getTargetPath(const std::string &filepath, const std::string &mime)
{
    std::string filename = std::filesystem::path(filepath).filename().string();

    if (mimetypes::isDocument(mime)) {
        return targetBase_ + "/documents/" + filename;
    }
    if (mimetypes::isImage(mime)) {
        return targetBase_ + "/images/" + filename;
    }
    if (mimetypes::isVideo(mime)) {
        return targetBase_ + "/videos/" + filename;
    }
    if (mimetypes::isAudio(mime)) {
        return targetBase_ + "/audio/" + filename;
    }
    if (mimetypes::isArchive(mime)) {
        return targetBase_ + "/archives/" + filename;
    }

    return targetBase_ + "/others/" + filename;
}

} // namespace aethercopy
