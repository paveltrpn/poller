module;

#include <concepts>
#include <algorithm>
#include <print>
#include <list>
#include <thread>
#include <mutex>
#include <memory>

export module container:list;
import :tag;

namespace poller {

export template <typename T, ConteinerTypeTag Type = THREAD_SAFE_BLOCK>
struct list;

export template <typename T>
struct list<T, THREAD_SAFE_BLOCK> final {
    using value_type = std::shared_ptr<T>;

    auto append( value_type item ) -> void {
        std::lock_guard _{ m_ };
        list_.push_back( item );
    }

    auto prepend( value_type item ) -> void {
        std::lock_guard _{ m_ };
        list_.push_front( item );
    }

    auto find_if( std::invocable<value_type> auto pred ) const
        -> std::list<std::shared_ptr<value_type>>::const_iterator {
        std::lock_guard _{ m_ };
        return std::find_if( list_.cbegin(), list_.cend(), pred );
    }

    auto find_if( std::invocable<value_type> auto pred )
        -> std::list<value_type>::iterator {
        std::lock_guard _{ m_ };
        return std::find_if( list_.begin(), list_.end(), pred );
    }

    auto for_each( std::invocable<value_type> auto pred ) const -> void {
        std::lock_guard _{ m_ };
        std::for_each( list_.cbegin(), list_.cend(), pred );
    }

    auto for_each( std::invocable<value_type> auto pred ) -> void {
        std::lock_guard _{ m_ };
        std::for_each( list_.begin(), list_.end(), pred );
    }

    auto begin() { return list_.begin(); }

    const auto cbegin() const { return list_.cbegin(); }

    auto end() { return list_.end(); }

    const auto cend() const { return list_.cend(); }

private:
    std::list<value_type> list_;
    std::mutex m_;
};

}  // namespace poller
