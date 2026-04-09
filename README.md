Консольная утилита для бекапирования файлов по выбранным MIME-типам.

## Зависимости(Linux)

- libmagic development headers
- libarchive
- CLI11 (headers)
- Abseil logging (absl/log)
- liburing

## Сборка и запуск тестов
```bash
cd build
cmake .. -DBUILD_TEST=ON
make aethercopy_tests
```

## Логирование
```bash
# Без логов (по умолчанию)
cmake -B build
cmake --build build

# С логами в библиотеке
cmake -B build -DENABLE_LOGGING_LIB=ON
cmake --build build

# С логами в тестах
cmake -B build -DENABLE_LOGGING_TESTS=ON
cmake --build build

# С логами везде
cmake -B build -DENABLE_LOGGING_LIB=ON -DENABLE_LOGGING_TESTS=ON
cmake --build build

```

## Сборка и запуск утилиты
```bash
$ cd build
$ cmake .. -DBUILD_TEST=OFF
$ make aethercopycli
$ ./aethercopycli --help

# боевой
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_LOGGING_LIB=OFF -DENABLE_LOGGING_TESTS=OFF
```

## Использование
```bash
# Только PDF и JPEG
./aethercopycli ~/Docs ~/backup --mime application/pdf --mime image/jpeg

# Офисные документы + ещё какой-то специфичный MIME
./aethercopycli ~/Docs ~/backup --office --mime application/x-some-custom

# Можно комбинировать с группами
./aethercopycli ~/Media ~/backup --images --videos --mime image/webp
```

## Аргументы
```
aethercopy [OPTIONS] source target
POSITIONALS:
  source TEXT:PATH(existing) REQUIRED
                              Source directory or file
  target TEXT REQUIRED        Target directory

OPTIONS:
  -h,     --help              Print this help message and exit
          --documents         Include office documents (PDF, DOCX, XLSX, PPTX)
          --images            Include images (JPEG, PNG, GIF, WEBP, etc)
          --videos            Include videos (MP4, MKV, WEBM, etc)
          --audio             Include audio (MP3, WAV, FLAC, etc)
          --archives          Include archives (ZIP, TAR, 7Z, RAR)
          --mime TEXT         Include specific MIME type (can be used multiple times)
          --all               Include all of this
  -V,     --verbose           Verbose output
  -r,     --recursive         Process recursively
  -t,     --threads INT:INT in [1 - 16] [4]  
                              Number of threads
  -e,     --entries UINT:INT in [1 - 4096] 
                              io_uring queue size (default: 512)
  -c,     --chunk-size UINT:SIZE [b, kb(=1000b), kib(=1024b), ...] 
                              Copy chunk size in bytes (default: 64MB, supports K/M/G suffix)
```

## Архитектура и как это работает

- `BackupEngine` — координирует копирование файлов и обработку архивов. В случае архивов он распаковывает во временную директорию и повторно обходит содержимое.
- `FormatDetector` — определяет MIME-тип файла с использованием libmagic (потокобезопасен через мьютексы).
- `FormatFilter` — контролирует, какие файлы копировать, на основе включения/исключения MIME-типов.
- `LibarchiveArchiveHandler` — занимается распаковкой архивов на диск.
- `ThreadPool` — обеспечивает асинхронность выполнения задач копирования.
- `SyncCopier` — копирование файлов с сохранением структуры директорий.
- CLI-слой (`CliHandler`) — парсит аргументы и конфигурирует фильтр через `FilterBuilder`.

Ключевые файлы (для быстрого изучения):
- `AetherCopy/include/aethercopy/mimeTypes.hpp` — набор MIME-типов и проверки типа
- `AetherCopy/src/mimeTypes.cpp` — определения MIME-типов
- `AetherCopy/include/aethercopy/FormatDetector.hpp` / `AetherCopy/src/FormatDetector.cpp` — детектор MIME через libmagic
- `AetherCopy/include/aethercopy/FormatFilter.hpp` / `AetherCopy/src/FormatFilter.cpp` — логика включения/исключения форматов
- `AetherCopy/src/BackupEngine.cpp` — обработка файлов и архивов
- `AetherCopy/src/FilterBuilder/FilterBuilder.cpp` — сбор фильтра по опциям CLI
- `AetherCopy/src/cli/CliHandler.cpp` / `CliHandler.hpp` — парсинг CLI и запуск
- `AetherCopy/src/ThreadPool.cpp` — пул потоков
- `AetherCopy/include/aethercopy/copiers/SyncCopier.hpp` / `SyncCopier.cpp` — копирование файлов
- `AetherCopy/include/aethercopy/copiers/IoURingCopier.hpp` / `IoURingCopier.cpp` — io_uring копировщик

## TODO
- [ ] Отдельные опции для конкретных популярных mime
- [ ] Распаковывать маленькие архивы в оперативной памяти
- [ ] Копирование через io_uring
  - [x] Базовый асинхронный копировщик
  - [ ] Дождаться версии liburing 2.16 и переписать на io_uring_prep_copy_file_range
  - [ ] Разобраться с исчерпанием SQ. Возможно в силу предыдущего пункта не понадобится
  - [ ] Прямые дескрипторы и SQPOLL
- [ ] Лог соответствия файла и его пути
- [ ] Опция сохранения исходной файловой структуры
