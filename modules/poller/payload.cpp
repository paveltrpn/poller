
module;

#include <string>
#include <functional>

export module poller:payload;

import :result;

namespace poller {

export using CallbackFn = std::function<void( Result result )>;

export struct Payload {
    CallbackFn callback;
    std::string data;
    std::string headers;
};

}  // namespace poller