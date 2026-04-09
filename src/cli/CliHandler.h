#ifndef AETHERCOPY_CLI_CLIHANDLER_H
#define AETHERCOPY_CLI_CLIHANDLER_H

#include "CLI/CLI.hpp"
#include <memory>
#include <string>

namespace aethercopy {
class BackupEngine;
}

namespace aethercopy::cli {

struct BackupOptions
{
    std::string source;
    std::string target;
    bool verbose{ false };
    bool recursive{ true };
    int threads{ 4 };

    bool includeDocuments{ false };
    bool includeImages{ false };
    bool includeVideos{ false };
    bool includeAudio{ false };
    bool includeArchives{ false };
    bool includeAll{ false };

    std::vector<std::string> includeMimes;

    size_t ring_entries{ 512 };
    size_t chunk_size{ 64 * 1024 * 1024 };
};

class CliHandler
{
  public:
    CliHandler();

    BackupOptions parse(int argc, char** argv);

    int run(const BackupOptions& opts, std::shared_ptr<BackupEngine> engine);
    void printHelp();
    void printVersion();

  private:
    CLI::App app_;
    BackupOptions opts_;
    bool initialized_{ false };
};

} // namespace aethercopy::cli

#endif // AETHERCOPY_CLI_CLIHANDLER_H
