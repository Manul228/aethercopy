#include "aethercopy/filter.h"
#include <algorithm>

namespace aethercopy {

void FormatFilter::includeOnly(const std::vector<std::string>& formats) {
    includeList_ = formats;
    includeMode_ = true;
}

void FormatFilter::exclude(const std::vector<std::string>& formats) {
    excludeList_ = formats;
    includeMode_ = false;
}

bool FormatFilter::shouldCopy(const std::string& mime_type) const {
    if (includeMode_) {
        // Включаем только то, что в списке
        return std::find(includeList_.begin(), includeList_.end(), mime_type) != includeList_.end();
    } else {
        // Исключаем то, что в списке
        return std::find(excludeList_.begin(), excludeList_.end(), mime_type) == excludeList_.end();
    }
}

} // namespace aethercopy