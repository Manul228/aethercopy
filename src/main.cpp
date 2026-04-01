#include <iostream>
#include <filesystem>
#include "aethercopy/detector.h"
#include "aethercopy/filter.h"
#include "thread_pool.h"
#include "async_copier.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <source_dir> <target_dir>\n";
        return 1;
    }

    std::string source_dir = argv[1];
    std::string target_dir = argv[2];

    // Создаём компоненты
    aethercopy::FormatDetector detector;
    aethercopy::FormatFilter filter;

    // Пример: копируем только изображения
    filter.includeOnly({"image/jpeg", "image/png"});

    // Ваш пул потоков и копировщик
    ThreadPool pool;
    pool.start(4);  // 4 потока

    AsyncFileCopier copier;

    // Сканируем файлы
    for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string mime = detector.detect(path);

            if (filter.shouldCopy(mime)) {
                std::string target = target_dir + "/" + entry.path().filename().string();

                pool.enqueue([&copier, path, target]() {
                    copier.submitCopy(path, target);
                });

                std::cout << "Queued: " << path << " (" << mime << ")\n";
            }
        }
    }

    // Ждём завершения
    pool.stop();
    copier.waitForCompletion();

    std::cout << "Done!\n";
    return 0;
}