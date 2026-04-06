#include "aethercopy/FormatDetector.hpp"
#include <magic.h>
#include <stdexcept>

using namespace aethercopy;

FormatDetector::FormatDetector() {
    // magic_open возвращает magic_t, а не void*
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    if (!magic) {
        throw std::runtime_error("Failed to initialize libmagic");
    }

    if (magic_load(magic, nullptr) != 0) {
        magic_close(magic);
        throw std::runtime_error("Failed to load magic database");
    }

    // Сохраняем как void* для сокрытия типа
    magic_cookie_ = static_cast<void*>(magic);
}

FormatDetector::~FormatDetector() {
    if (magic_cookie_) {
        magic_close(static_cast<magic_t>(magic_cookie_));
    }
}

std::string FormatDetector::detect(const std::string& filepath) {
    // libmagic не потокобезопасен
    std::lock_guard<std::mutex> lock(magicMutex_);
    magic_t                     magic = static_cast<magic_t>(magic_cookie_);
    const char* mime = magic_file(magic, filepath.c_str());

    if (!mime) {
        return "application/octet-stream";
    }

    return std::string(mime);
}