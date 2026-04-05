#ifndef AETHERCOPY_DETECTOR_H
#define AETHERCOPY_DETECTOR_H

#include <mutex>
#include <string>

namespace aethercopy {

class FormatDetector {
public:
    FormatDetector();
    ~FormatDetector();

    // Возвращает MIME тип файла (например, "image/jpeg")
    std::string detect(const std::string& filepath);

private:
    void *magic_cookie_;
    std::mutex magicMutex_;
};

} // namespace aethercopy

#endif