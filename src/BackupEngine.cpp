#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <iostream>

#include "aethercopy/BackupEngine.h"
#include "aethercopy/detector.h"
#include "aethercopy/mimeTypes.h"

using namespace aethercopy;

namespace fs = std::filesystem;

BackupEngine::BackupEngine(ThreadPool &pool, ICopier &copier, FormatFilter &filter,
                           IArchiveHandler &handler, const std::string &targetBase,
                           const std::string &tempDir)
    : pool_(pool), copier_(copier), filter_(filter), archiveHandler_(handler),
      targetBase_(targetBase), tempDir_(tempDir)
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
            std::string tempDir =
                self->tempDir_ + std::to_string(time(nullptr)) + std::to_string(std::rand());
            self->archiveHandler_.extractToDisk(path, tempDir);
            self->processDirectory(tempDir);
            // self->removeTempDir(tempDir);
        });
    } else {
        pool_.enqueue([self, path, mime]() {
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
        // TODO: добавить лог исходных путей файлов
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
