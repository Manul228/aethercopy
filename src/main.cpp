#include "aethercopy/ArchiveHandlers/LibarchiveArchiveHandler.h"
#include "aethercopy/BackupEngine.h"
#include "aethercopy/FormatFilter.hpp"
#include "aethercopy/ThreadPool.h"
#include "aethercopy/copiers/SyncCopier.h"
#include "cli/CliHandler.h"
#include <memory>

int main(int argc, char **argv)
{
    using namespace aethercopy;

    cli::CliHandler cli;
    auto opts = cli.parse(argc, argv);

    // Создаём зависимости
    auto pool = std::make_shared<ThreadPool>(opts.threads);
    auto copier = std::make_shared<SyncCopier>();
    auto handler = std::make_shared<LibarchiveArchiveHandler>();
    FormatFilter filter;
    filter.includeOnly({}); // всё копируем

    auto engine = std::make_shared<BackupEngine>(pool, copier, handler, filter, opts.target,
                                                 "/tmp/aethercopy");

    return cli.run(opts, engine);
}