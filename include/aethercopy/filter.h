#ifndef AETHERCOPY_FILTER_H
#define AETHERCOPY_FILTER_H

#include <string>
#include <vector>

namespace aethercopy {

// Простой фильтр форматов
class FormatFilter {
public:
    // Включаем только указанные форматы
    void includeOnly(const std::vector<std::string>& formats);

    // Исключаем указанные форматы
    void exclude(const std::vector<std::string>& formats);

    // Проверяем, нужно ли копировать файл
    bool shouldCopy(const std::string& mime_type) const;

private:
    std::vector<std::string> includeList_;
    std::vector<std::string> excludeList_;
    bool includeMode_{false}; // true = include only, false = exclude
};

} // namespace aethercopy

#endif