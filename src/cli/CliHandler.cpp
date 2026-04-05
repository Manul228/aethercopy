#include "CliHandler.h"
#include "absl/log/log.h"
#include "aethercopy/BackupEngine.h"
#include <CLI/CLI.hpp>

namespace aethercopy::cli {

CliHandler::CliHandler() : app_{ "AetherCopy - blazingly fast async backup tool" }
{
    app_.add_option("source", opts_.source, "Source directory or file")
        ->required()
        ->check(CLI::ExistingPath);

    app_.add_option("target", opts_.target, "Target directory")->required();

    app_.add_flag("-V,--verbose", opts_.verbose, "Verbose output");
    app_.add_flag("-r,--recursive", opts_.recursive, "Process recursively");

    int maxThreads = std::thread::hardware_concurrency();
    app_.add_option("-t,--threads", opts_.threads, "Number of threads")
        ->default_val(4)
        ->check(CLI::Range(1, maxThreads));
}

BackupOptions CliHandler::parse(int argc, char **argv)
{
    try {
        app_.parse(argc, argv);
    }
    catch (const CLI::ParseError &e) {
        exit(app_.exit(e));
    }

    return opts_;
}

int CliHandler::run(const BackupOptions &opts, std::shared_ptr<BackupEngine> engine)
{
    if (!engine) {
        DLOG(ERROR) << "BackupEngine not initialized\n";
        return 1;
    }

    try {
        if (opts.recursive) {
            engine->processDirectory(opts.source);
        }
        else {
            engine->processFile(opts.source);
        }

        engine->wait();

        if (opts.verbose) {
            DLOG(INFO) << "Backup completed successfully\n";
        }

        return 0;
    }
    catch (const std::exception &e) {
        DLOG(ERROR) << e.what() << '\n';
        return 1;
    }
    return 0;
}

void CliHandler::printHelp()
{
    std::cout << "AetherCopy v1.0.0\n"
              << "Usage: aethercopycli [options] <source> <target>\n"
              << "  -V, --verbose    Verbose output\n"
              << "  -r, --recursive  Process recursively\n"
              << "  -t, --threads N  Number of threads (default: 4)\n"
              << "  -h, --help       Show this help\n"
              << "  -v, --version    Show version\n";
}

void CliHandler::printVersion()
{
    std::cout << "AetherCopy version 1.0.0\n";
}

} // namespace aethercopy::cli