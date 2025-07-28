
#include <print>

import io;

auto main( int argc, char** argv ) -> int {
    poller::io::File sched{};

    sched.run();
    sched.sync_wait();

    return 0;
}
