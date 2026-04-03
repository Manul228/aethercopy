#include <archive.h>
#include <archive_entry.h>
#include <iostream>

#include "aethercopy/archive_processor.h"
#include "aethercopy/detector.h"
#include "aethercopy/mime_types.h"

namespace aethercopy {

ArchiveProcessor::ArchiveProcessor(ThreadPool &p, AsyncFileCopier &copier)
    : pool_(p)
    , copier_(copier)
{}

void ArchiveProcessor::processFile(const std::string &path)
{
    std::string mime = detector_.detect(path);

    if (mimetypes::isArchive(mime)) {
        // чтобы класс пережил лямбду
        auto self = shared_from_this();
        pool_.enqueue([self, path]() {

        });
    }
}

bool ArchiveProcessor::isSmallArchive(const std::string &path)
{
    int64_t contentSize = getUncompressedArchiveSize(path);
    // TODO: что если содержимое архива размером 0?
    return contentSize < cutoffSize_;
}

int64_t ArchiveProcessor::getUncompressedArchiveSize(const std::string &archivePath)
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

    // Проверка, завершилось ли правильно
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
