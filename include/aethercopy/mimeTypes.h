#ifndef AETHERCOPY_MIME_TYPES_H
#define AETHERCOPY_MIME_TYPES_H

#include <string>
#include <unordered_set>

namespace aethercopy {
namespace mimetypes {

// Списки MIME типов
extern const std::unordered_set<std::string> ARCHIVE_MIMES;
extern const std::unordered_set<std::string> IMAGE_MIMES;
extern const std::unordered_set<std::string> VIDEO_MIMES;
extern const std::unordered_set<std::string> AUDIO_MIMES;
extern const std::unordered_set<std::string> DOCUMENT_MIMES;

// Функции проверки
bool isArchive(const std::string &mime);
bool isImage(const std::string &mime);
bool isVideo(const std::string &mime);
bool isAudio(const std::string &mime);
bool isDocument(const std::string &mime);

} // namespace mimetypes
} // namespace aethercopy

#endif