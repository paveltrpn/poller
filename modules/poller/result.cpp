
module;

#include <string>

export module poller:result;

namespace poller {

export struct Result {
    long code;
    std::string data;
    std::string headers;
};

}  // namespace poller