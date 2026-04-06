Консольная утилита для бекапирования файлов по выбранным MIME-типам.

## Зависимости(Linux)

- libmagic development headers
- libarchive
- CLI11 (headers)
- Abseil logging (absl/log)

## Сборка и запуск тестов
```bash
cd build
cmake .. -DBUILD_TEST=ON
make aethercopy_tests
```

## Сборка и запуск утилиты
```bash
$ cd build
$ cmake .. -DBUILD_TEST=OFF
$ make aethercopycli
$ ./aethercopycli --help
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
- `AetherCopy/include/aethercopy/mimeTypes.hpp` — наборы MIME-типов и проверки типа
- `AetherCopy/src/mimeTypes.cpp` — определения MIME-типов
- `AetherCopy/src/FormatDetector.cpp/.hpp` — детектор MIME через libmagic
- `AetherCopy/src/FormatFilter.cpp/.hpp` — логика включения/исключения форматов
- `AetherCopy/src/BackupEngine.cpp` — обработка файлов и архивов
- `AetherCopy/src/FilterBuilder/FilterBuilder.cpp` — сбор фильтра по опциям CLI
- `AetherCopy/src/cli/CliHandler.cpp/.h` — парсинг CLI и запуск
- `AetherCopy/src/ThreadPool.cpp` — пул потоков
- `AetherCopy/src/copiers/SyncCopier.cpp` — копирование файлов

## TODO
- [ ] Распаковывать маленькие архивы в оперативной памяти
- [ ] Копирование через io_uring
- [ ] Лог соответствия файла и его пути
- [ ] Опция сохранения исходной файловой структуры
