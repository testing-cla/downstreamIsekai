
#include <envirdefs.h>

#include <cstdint>

#include "glog/logging.h"

namespace omnetpp {
namespace envir {
extern "C" ENVIR_API int evMain(int argc, char* argv[]);
}  // namespace envir
}  // namespace omnetpp

int main(int argc, char** argv) { return omnetpp::envir::evMain(argc, argv); }
