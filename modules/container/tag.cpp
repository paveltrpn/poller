module;

#include <print>
#include <type_traits>

export module container:tag;

namespace poller {

struct THREAD_UNSAFE;
struct THREAD_SAFE_BLOCK;
struct THREAD_SAFE_NBLOCK;

template <typename T>
concept ContainerTypeTag =
    std::is_same_v<T, THREAD_UNSAFE> || std::is_same_v<T, THREAD_SAFE_BLOCK> ||
    std::is_same_v<T, THREAD_SAFE_NBLOCK>;

}  // namespace poller