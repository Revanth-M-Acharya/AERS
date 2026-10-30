/**
 * @file QnxDevctl.cpp
 * @brief Implementation of Phase 4 QNX devctl dispatching.
 */

#include "aegis/qnx/QnxDevctl.hpp"
#include <cerrno>

#if defined(__QNX__) || defined(__QNXNTO__)
    #include <devctl.h>
    #include <unistd.h>
#else
    // Forward declare hook to simulated device manager
    namespace aegis {
    namespace qnx {
        int SimulatedDevctlDispatcher(int fd, int dcmd, void* dataPtr, size_t nBytes);
    }
    }
#endif

namespace aegis {
namespace qnx {

int InvokeDevctl(int fd, int dcmd, void* dataPtr, size_t nBytes, int* infoPtr) {
#if defined(__QNX__) || defined(__QNXNTO__)
    return devctl(fd, dcmd, dataPtr, nBytes, infoPtr);
#else
    (void)infoPtr;
    return SimulatedDevctlDispatcher(fd, dcmd, dataPtr, nBytes);
#endif
}

} // namespace qnx
} // namespace aegis

