#include "FilterBuilder.hpp"
#include "aethercopy/mimeTypes.hpp"

using namespace aethercopy;
using namespace mimetypes;

void FilterBuilder::buildFilterFromOptions(FormatFilter& filter,
                                           const cli::BackupOptions& opts)
{
    std::vector<std::string> includeList;

    // auto и для set и для vector
    auto addToIncludeList = [&](const auto& mimeSet, bool included) {
        if (included)
            includeList.insert(
                includeList.end(), mimeSet.begin(), mimeSet.end());
    };

    addToIncludeList(DOCUMENT_MIMES, opts.includeDocuments);
    addToIncludeList(IMAGE_MIMES, opts.includeImages);
    addToIncludeList(ARCHIVE_MIMES, opts.includeArchives);
    addToIncludeList(AUDIO_MIMES, opts.includeAudio);
    addToIncludeList(VIDEO_MIMES, opts.includeVideos);

    addToIncludeList(opts.includeMimes, !opts.includeMimes.empty());

    if (opts.includeAll || (includeList.empty() && opts.includeMimes.empty())) {
        filter.includeOnly({});
    } else {
        std::sort(includeList.begin(), includeList.end());
        auto last = std::unique(includeList.begin(), includeList.end());
        includeList.erase(last, includeList.end());
        filter.includeOnly(includeList);
    }
}