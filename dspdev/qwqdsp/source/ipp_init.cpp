#include "ipp_init.hpp"

#include <mutex>
#include <ipp.h>

namespace qwqdsp::internal {
static std::once_flag ipp_init_flag;
void InitIPPOnce() {
    std::call_once(ipp_init_flag, []{
        ippInit();
    });
}
}