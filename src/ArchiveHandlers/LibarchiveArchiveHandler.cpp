#include "aethercopy/ArchiveHandlers/LibarchiveArchiveHandler.h"
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>

namespace aethercopy {

std::string LibarchiveArchiveHandler::extractToDisk(const std::string &archivePath,
                                                    const std::string &targetPath)
{
    std::string tempDir = targetPath + std::to_string(time(nullptr)) + std::to_string(std::rand());

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

int64_t LibarchiveArchiveHandler::getUncompressedArchiveSize(const std::string &archivePath)
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