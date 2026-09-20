#include "ipc/shared_memory_layout.hpp"

namespace fast_channel {

void initialize_control(SharedMemoryControl& control) {
    control.empty.initialize(false);
    control.full.initialize(true);
}

}
