#ifndef AETHERCOPY_DETECTOR_H
#define AETHERCOPY_DETECTOR_H

#include <string>

namespace aethercopy {

// Простой детектор формата файла через libmagic
class FormatDetector {
public:
    FormatDetector();
    ~FormatDetector();

    // Возвращает MIME тип файла (например, "image/jpeg")
    std::string detect(const std::string& filepath);

private:
    void* magic_cookie_;  // opaque pointer to magic_t
};

} // namespace aethercopy

#endif