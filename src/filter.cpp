#include "aethercopy/filter.h"
#include <algorithm>

namespace aethercopy {

void FormatFilter::includeOnly(const std::vector<std::string>& formats) {
    include_list_ = formats;
    include_mode_ = true;
}

void FormatFilter::exclude(const std::vector<std::string>& formats) {
    exclude_list_ = formats;
    include_mode_ = false;
}

bool FormatFilter::shouldCopy(const std::string& mime_type) const {
    if (include_mode_) {
        // Включаем только то, что в списке
        return std::find(include_list_.begin(), include_list_.end(), mime_type)
               != include_list_.end();
    } else {
        // Исключаем то, что в списке
        return std::find(exclude_list_.begin(), exclude_list_.end(), mime_type)
               == exclude_list_.end();
    }
}

} // namespace aethercopy