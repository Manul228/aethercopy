#ifndef FILTERBUILDER_HPP
#define FILTERBUILDER_HPP

#include "../cli/CliHandler.h"
#include "aethercopy/FormatFilter.hpp"

namespace aethercopy {

class FilterBuilder
{
  public:
    static void buildFilterFromOptions(FormatFilter& filter,
                                       const cli::BackupOptions& opts);
};

} // namespace aethercopy

#endif // FILTERBUILDER_HPP
