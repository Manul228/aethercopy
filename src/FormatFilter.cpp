#include "aethercopy/FormatFilter.hpp"

#include <algorithm>

using namespace aethercopy;

void FormatFilter::includeOnly(const std::vector<std::string>& formats) {
    includeList_ = formats;
    includeMode_ = true;
    excludeList_.clear();
}

void FormatFilter::exclude(const std::vector<std::string>& formats) {
    excludeList_ = formats;
    includeMode_ = false;
}

bool FormatFilter::shouldCopy(const std::string& mime_type) const {
    if (includeMode_) {
        // Пустой список = всё копируем
        if (includeList_.empty()) {
            return true;
        }
        return std::find(includeList_.begin(), includeList_.end(), mime_type) != includeList_.end();
    } else {
        // Пустой список = ничего не исключаем
        if (excludeList_.empty()) {
            return true;
        }
        return std::find(excludeList_.begin(), excludeList_.end(), mime_type) == excludeList_.end();
    }
}