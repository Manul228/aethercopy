#include "FilterBuilder/FilterBuilder.hpp"
#include "aethercopy/ArchiveHandlers/LibarchiveArchiveHandler.h"
#include "aethercopy/BackupEngine.h"
#include "aethercopy/FormatFilter.hpp"
#include "aethercopy/ThreadPool.h"
#include "aethercopy/copiers/IoURingCopier.hpp"
#include "aethercopy/copiers/SyncCopier.h"
#include "cli/CliHandler.h"
#include <memory>

bool ioUringAvailable()
{
    int fd = syscall(__NR_io_uring_setup, 1, nullptr);
    if (fd < 0)
    {
        return errno != ENOSYS;
    }
    close(fd);
    return true;
}

int main(int argc, char** argv)
{
    using namespace aethercopy;

    cli::CliHandler cli;
    auto opts = cli.parse(argc, argv);

    auto pool = std::make_shared<ThreadPool>(opts.threads);

    std::shared_ptr<ICopier> copier;

    if (ioUringAvailable())
    {
        copier =
            std::make_shared<IoURingCopier>(opts.ring_entries, opts.chunk_size);
    }
    else
    {
        copier = std::make_shared<SyncCopier>();
    }

    auto handler = std::make_shared<LibarchiveArchiveHandler>();
    FormatFilter filter;
    FilterBuilder::buildFilterFromOptions(filter, opts);

    auto engine = std::make_shared<BackupEngine>(
        pool, copier, handler, filter, opts.target, "/tmp/aethercopy");

    return cli.run(opts, engine);
}