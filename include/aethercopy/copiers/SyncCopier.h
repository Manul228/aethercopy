#ifndef SYNCCOPIER_H
#define SYNCCOPIER_H

#include "ICopier.h"

namespace aethercopy {

class SyncCopier : public ICopier
{
    // ICopier interface
public:
    void copy(const std::string &src, const std::string &dst) override;
};
} // namespace aethercopy

#endif // SYNCCOPIER_H
