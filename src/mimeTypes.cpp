// src/mime_types.cpp
#include "aethercopy/mimeTypes.h"

namespace aethercopy {
namespace mimetypes {
const std::unordered_set<std::string> ARCHIVE_MIMES = {"application/zip",
                                                       "application/x-zip-compressed",
                                                       "application/x-tar",
                                                       "application/x-gzip",
                                                       "application/gzip",
                                                       "application/x-bzip2",
                                                       "application/x-xz",
                                                       "application/x-7z-compressed",
                                                       "application/x-rar",
                                                       "application/x-rar-compressed",
                                                       "application/x-cpio",
                                                       "application/x-shar",
                                                       "application/x-iso9660-image",
                                                       "application/vnd.rar"};

const std::unordered_set<std::string> IMAGE_MIMES = {"image/jpeg",
                                                     "image/png",
                                                     "image/gif",
                                                     "image/webp",
                                                     "image/svg+xml",
                                                     "image/bmp",
                                                     "image/tiff",
                                                     "image/x-icon",
                                                     "image/heic",
                                                     "image/heif",
                                                     "image/avif"};

const std::unordered_set<std::string> VIDEO_MIMES = {"video/mp4",
                                                     "video/x-matroska",
                                                     "video/webm",
                                                     "video/ogg",
                                                     "video/quicktime",
                                                     "video/x-msvideo",
                                                     "video/x-flv",
                                                     "video/mpeg",
                                                     "video/3gpp"};

const std::unordered_set<std::string> AUDIO_MIMES = {"audio/mpeg",
                                                     "audio/ogg",
                                                     "audio/flac",
                                                     "audio/wav",
                                                     "audio/x-wav",
                                                     "audio/webm",
                                                     "audio/aac",
                                                     "audio/mp4",
                                                     "audio/opus"};

const std::unordered_set<std::string> DOCUMENT_MIMES
    = {"application/pdf",
       "text/plain",
       "text/html",
       "text/xml",
       "application/msword",
       "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
       "application/vnd.ms-excel",
       "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
       "application/vnd.ms-powerpoint",
       "application/vnd.openxmlformats-officedocument.presentationml.presentation",
       "text/markdown",
       "application/json",
       "application/xml"};

bool isArchive(const std::string &mime)
{
    return ARCHIVE_MIMES.find(mime) != ARCHIVE_MIMES.end();
}

bool isImage(const std::string &mime)
{
    return IMAGE_MIMES.find(mime) != IMAGE_MIMES.end();
}

bool isVideo(const std::string &mime)
{
    return VIDEO_MIMES.find(mime) != VIDEO_MIMES.end();
}

bool isAudio(const std::string &mime)
{
    return AUDIO_MIMES.find(mime) != AUDIO_MIMES.end();
}

bool isDocument(const std::string &mime)
{
    return DOCUMENT_MIMES.find(mime) != DOCUMENT_MIMES.end();
}

} // namespace mimetypes
} // namespace aethercopy