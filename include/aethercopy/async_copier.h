#ifndef AETHERCOPY_ASYNC_FILE_COPIER_H
#define AETHERCOPY_ASYNC_FILE_COPIER_H

#include <cstring>
#include <liburing.h>
#include <string>
#include <vector>

namespace aethercopy {

struct Buffer
{
    char *data = nullptr;
    off_t offset = 0;
    size_t length = 0;
    bool in_use = false;
};

class AsyncFileCopier
{
public:
    /**
     * Конструктор
     * @param buf_size Размер буфера для чтения/записи
     * @param queue_depth Глубина очереди операций
     * @throws std::runtime_error при ошибке инициализации io_uring или выделения памяти
     */
    AsyncFileCopier(size_t buf_size, int queue_depth);

    /**
     * Деструктор - освобождает все ресурсы
     */
    ~AsyncFileCopier();

    /**
     * Асинхронное копирование файла
     * @param src Путь к исходному файлу
     * @param dst Путь к целевому файлу
     * @throws std::runtime_error при ошибках копирования
     */
    void copy_file(const std::string &src, const std::string &dst);

    // Запрещаем копирование
    AsyncFileCopier(const AsyncFileCopier &) = delete;
    AsyncFileCopier &operator=(const AsyncFileCopier &) = delete;

    // Запрещаем перемещение
    AsyncFileCopier(AsyncFileCopier &&other) = delete;
    AsyncFileCopier &operator=(AsyncFileCopier &&other) = delete;

private:
    /**
     * Отправка запроса на чтение
     * @return true если запрос был отправлен, false если нет
     */
    bool submitRead(int fd_in, off_t file_size, off_t &read_offset, int &inflight);

    /**
     * Отправка запроса на запись
     */
    bool submitWrite(int fd_out, int idx, size_t bytes);

    /**
     * Обработка завершенной операции
     */
    void handleCqe(int fd_out, off_t &write_offset, int &inflight, struct io_uring_cqe *cqe);

    /**
     * Поиск свободного буфера
     * @return индекс буфера или -1 если нет свободных
     */
    int findFreeBuffer();

    /**
     * Очистка всех буферов
     */
    void cleanupBuffers();

    size_t bufSize_;
    int queueDepth_;
    struct io_uring ring_;
    bool ringInited_;
    std::vector<Buffer> buffers_;
};

} // namespace aethercopy

#endif // AETHERCOPY_ASYNC_FILE_COPIER_H