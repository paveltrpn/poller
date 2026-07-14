module;

#include <atomic>
#include <cassert>
#include <optional>
#include <thread>
#include <vector>

export module poller_std:array;

namespace poller {

// The `Array` class is copied from Taskflow licensed under the MIT License.
// Original code:
// https://github.com/taskflow/taskflow/blob/master/taskflow/core/tsq.hpp
template <typename T>
    requires std::is_pointer_v<T>
class Array {
public:
    explicit Array( const int capacity )
        : capacity_{ capacity }
        , mask_{ capacity - 1 }
        , buffer_{ new std::atomic<T>[capacity] } {}

    Array( const Array & ) = delete;
    Array( Array && ) = delete;
    Array &operator=( const Array & ) = delete;
    Array &operator=( Array && ) = delete;
    ~Array() noexcept { delete[] buffer_; }

    void Put( const size_t index, T item ) noexcept { buffer_[index & mask_].store( item, std::memory_order_relaxed ); }

    [[nodiscard]] T Get( const size_t index ) noexcept {
        return buffer_[index & mask_].load( std::memory_order_relaxed );
    }

    [[nodiscard]] Array *Resize( const size_t bottom, const size_t top ) {
        auto *array = new Array{ 2 * capacity_ };
        for ( auto i = top; i != bottom; ++i ) {
            array->Put( i, Get( i ) );
        }
        return array;
    }

    [[nodiscard]] int Capacity() const { return capacity_; }

private:
    const int capacity_, mask_;
    std::atomic<T> *buffer_;
};

}  // namespace poller
