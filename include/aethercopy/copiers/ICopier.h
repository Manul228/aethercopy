#ifndef ICOPIER_H
#define ICOPIER_H

#include <string>

namespace aethercopy {

class ICopier
{
public:
    virtual ~ICopier() = default;

    virtual void copy(const std::string &src, const std::string &dst) = 0;

    virtual void wait();
};

} // namespace aethercopy
#endif // ICOPIER_H
