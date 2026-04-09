#include "CliHandler.h"
#include "absl/log/log.h"
#include "aethercopy/BackupEngine.h"
#include <CLI/CLI.hpp>

namespace aethercopy::cli {

CliHandler::CliHandler()
    : app_{ "AetherCopy - blazingly fast async backup tool" }
{
    app_.add_option("source", opts_.source, "Source directory or file")
        ->required()
        ->check(CLI::ExistingPath);

    app_.add_option("target", opts_.target, "Target directory")->required();

    // Групповые флаги
    app_.add_flag("--documents",
                  opts_.includeDocuments,
                  "Include office documents (PDF, DOCX, XLSX, PPTX)");

    app_.add_flag("--images",
                  opts_.includeImages,
                  "Include images (JPEG, PNG, GIF, WEBP, etc)");

    app_.add_flag("--videos",
                  opts_.includeVideos,
                  "Include videos (MP4, MKV, WEBM, etc)");

    app_.add_flag(
        "--audio", opts_.includeAudio, "Include audio (MP3, WAV, FLAC, etc)");

    app_.add_flag("--archives",
                  opts_.includeArchives,
                  "Include archives (ZIP, TAR, 7Z, RAR)");

    app_.add_option("--mime",
                    opts_.includeMimes,
                    "Include specific MIME type (can be used multiple times)")
        ->expected(1, 20);

    app_.add_flag("--all", opts_.includeAll, "Include all of this");

    app_.add_flag("-V,--verbose", opts_.verbose, "Verbose output");
    app_.add_flag("-r,--recursive", opts_.recursive, "Process recursively");

    int maxThreads = std::thread::hardware_concurrency();
    app_.add_option("-t,--threads", opts_.threads, "Number of threads")
        ->default_val(4)
        ->check(CLI::Range(1, maxThreads));

    app_.add_option(
            "-e, --entries",
            opts_.ring_entries,
            "io_uring queue size (default: 512)"
    )
        ->check(CLI::Range(1, 4096));

    // chunk_size — размер куска для копирования
    app_.add_option(
            "-c, --chunk-size",
            opts_.chunk_size,
            "Copy chunk size in bytes (default: 64MB, supports K/M/G suffix)"
    )
        ->transform(CLI::AsSizeValue(true));
}

BackupOptions CliHandler::parse(int argc, char** argv)
{
    try {
        app_.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        exit(app_.exit(e));
    }

    return opts_;
}

int CliHandler::run(const BackupOptions& opts,
                    std::shared_ptr<BackupEngine> engine)
{
    if (!engine) {
        DLOG(ERROR) << "BackupEngine not initialized\n";
        return 1;
    }

    try {
        if (opts.recursive) {
            engine->processDirectory(opts.source);
        } else {
            engine->processFile(opts.source);
        }

        engine->wait();

        if (opts.verbose) {
            DLOG(INFO) << "Backup completed successfully\n";
        }

        return 0;
    } catch (const std::exception& e) {
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