
module;

#include <cstdlib>
#include <coroutine>

#include <uv.h>

export module io:timer;

import :schedulerbase;
import :async;

namespace poller::io {}