#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <iostream>

#include "aethercopy/BackupEngine.h"
#include "aethercopy/detector.h"
#include "aethercopy/mime_types.h"

namespace aethercopy {

BackupEngine::BackupEngine(ThreadPool &pool,
                           ICopier &copier,
                           FormatFilter &filter,
                           const std::string &targetBase)
    : pool_(pool)
    , copier_(copier)
    , filter_(filter)
    , targetBase_(targetBase)
{}

void BackupEngine::processFile(const std::string &path)
{
    std::string mime = detector_.detect(path);
    if (not filter_.shouldCopy(mime))
        return;

    auto self = shared_from_this();

    if (mimetypes::isArchive(mime)) {
        // чтобы класс пережил лямбду
        pool_.enqueue([self, path]() {
            std::string tempDir = self->extractToDisk(path);
            self->processDirectory(tempDir);
            self->removeTempDir(tempDir);
        });
    } else {
        pool_.enqueue([self, path, &mime]() {
            //
            self->copier_.copy(path, self->getTargetPath(path, mime));
        });
    }
}

void BackupEngine::processDirectory(const std::string &dirPath)
{
    try {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dirPath)) {
            if (entry.is_regular_file())
                processFile(entry.path().string());
        }
    } catch (const std::filesystem::filesystem_error &e) {
        std::clog << "[ERROR] [processDirectory] :" << e.what() << '\n';
    }
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

std::string BackupEngine::extractToDisk(const std::string &archivePath)
{
    std::string tempDir = tempDir_ + std::to_string(std::time(nullptr))
                          + std::to_string(std::rand());

    namespace fs = std::filesystem;

    mkdir(tempDir.c_str(), 0755);

    struct archive *a = archive_read_new();
    struct archive *ext = archive_write_disk_new();
    struct archive_entry *entry;

    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    // 10240 из документации взято
    const size_t HEADER_BUFFER_SIZE = 10240;

    if (archive_read_open_filename(a, archivePath.c_str(), HEADER_BUFFER_SIZE) != ARCHIVE_OK) {
        archive_read_free(a);
        archive_write_free(ext);
        throw std::runtime_error("Failed to open archive when extraction");
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *entryName = archive_entry_pathname(entry);
        std::string fullPath = tempDir + "/" + entryName;

        archive_entry_set_pathname(entry, fullPath.c_str());

        if (archive_write_header(ext, entry) != ARCHIVE_OK)
            continue;
    }

    const void *buff;
    size_t size;
    la_int64_t offset;

    while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
        archive_write_data_block(ext, buff, size, offset);
    }

    archive_read_free(a);
    archive_write_free(ext);

    return tempDir;
}

bool BackupEngine::isSmallArchive(const std::string &path)
{
    int64_t contentSize = getUncompressedArchiveSize(path);
    // TODO: что если содержимое архива размером 0?
    return contentSize < cutoffSize_;
}

int64_t BackupEngine::getUncompressedArchiveSize(const std::string &archivePath)
{
    int64_t total_size = 0;

    struct archive *a = archive_read_new();
    if (!a) {
        std::cerr << "Failed to create archive object\n";
        return -1;
    }

    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    int r = archive_read_open_filename(a, archivePath.c_str(), 10240);
    if (r != ARCHIVE_OK) {
        std::cerr << "Failed to open archive: " << archive_error_string(a) << '\n';
        archive_read_free(a);
        return -1;
    }

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        // Пропускаем директории, их размер 0, но их
        // содержимое учитывается таким перебором.
        if (archive_entry_filetype(entry) != AE_IFDIR) {
            int64_t size = archive_entry_size(entry);
            if (size >= 0) {
                total_size += size;
            }
        }
        archive_read_data_skip(a); // пропускаем содержимое
    }

    // Конец архива
    if (r != ARCHIVE_EOF && r != ARCHIVE_OK) {
        std::cerr << "Error reading archive: " << archive_error_string(a) << '\n';
        total_size = -1;
    }

    r = archive_read_free(a);
    if (r != ARCHIVE_OK) {
        std::cerr << "archive_read_free failed: " << archive_error_string(a) << '\n';
    }

    return total_size;
}

} // namespace aethercopy
