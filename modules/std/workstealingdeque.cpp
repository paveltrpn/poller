
module;

#include <atomic>
#include <cassert>
#include <functional>
#include <new>
#include <optional>
#include <thread>
#include <vector>

export module poller_std:workstealingdeque;

import :array;

namespace poller {

// The `WorkStealingDeque` class is copied from Google Filament licensed under
// the Apache License 2.0.
// Original code:
// https://github.com/google/filament/blob/main/libs/utils/include/utils/WorkStealingDequeue.h
template <typename T>
    requires std::is_pointer_v<T>
class WorkStealingDeque {
public:
    explicit WorkStealingDeque( const int capacity = 1024 )
        : top_{ 0 }
        , bottom_{ 0 }
        , array_{ new Array<T>{ capacity } } {
        assert( capacity && ( capacity & capacity - 1 ) == 0 );
        garbage_.reserve( 64 );
    }

    WorkStealingDeque( const WorkStealingDeque & ) = delete;
    WorkStealingDeque( WorkStealingDeque && ) = delete;
    WorkStealingDeque &operator=( const WorkStealingDeque & ) = delete;
    WorkStealingDeque &operator=( WorkStealingDeque && ) = delete;

    ~WorkStealingDeque() noexcept {
        for ( auto *array : garbage_ ) {
            delete array;
        }
        delete array_.load();
    }

    void Push( T item ) {
        // std::memory_order_relaxed is sufficient because this load doesn't acquire
        // anything from another thread. bottom_ is only written in pop() which
        // cannot be concurrent with push().
        const auto bottom = bottom_.load( std::memory_order_relaxed );
        const auto top = top_.load( std::memory_order_acquire );
        auto *array = array_.load( std::memory_order_relaxed );
        if ( array->Capacity() - 1 < bottom - top ) {
            array = Resize( array, bottom, top );
        }
        array->Put( bottom, item );
        // std::memory_order_release is used because we release the item we just
        // pushed to other threads which are calling steal().
        bottom_.store( bottom + 1, std::memory_order_release );
    }

    [[nodiscard]] T Pop() {
        // std::memory_order_seq_cst is needed to guarantee ordering in steal() Note
        // however that this is not a typical acquire/release operation:
        // - Not acquire because bottom_ is only written in push() which is not
        // concurrent.
        // - Not release because we're not publishing anything to steal() here.
        // QUESTION: does this prevent top_ load below to be reordered before the
        // "store" part of fetch_sub()? Hopefully it does. If not we'd need a full
        // memory barrier.
        const auto bottom = bottom_.fetch_sub( 1, std::memory_order_seq_cst ) - 1;
        // bottom could be -1 if we tried to pop() from an empty queue. This will be
        // corrected below.
        assert( bottom >= -1 );
        auto *array = array_.load( std::memory_order_relaxed );
        // std::memory_order_seq_cst is needed to guarantee ordering in steal().
        // Note however that this is not a typical acquire operation (i.e. other
        // thread's writes of top_ don't publish data).
        auto top = top_.load( std::memory_order_seq_cst );
        if ( top < bottom ) {
            // The queue isn't empty, and it's not the last item, just return it, this
            // is the common case.
            return array->Get( bottom );
        }
        T item{ nullptr };
        if ( top == bottom ) {
            // We just took the last item.
            item = array->Get( bottom );
            // Because we know we took the last item, we could be racing with steal()
            // -- the last item being both at the top and bottom of the queue. We
            // resolve this potential race by also stealing that item from ourselves.
            if ( top_.compare_exchange_strong( top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed ) ) {
                // Success: we stole our last item from ourselves, meaning that a
                // concurrent steal() would have failed. top_ now equals top + 1, we
                // adjust top to make the queue empty.
                ++top;
            } else {
                // Failure: top_ was not equal to top, which means the item was stolen
                // under our feet. top now equals to top_. Simply discard the item we
                // just popped. The queue is now empty.
                item = nullptr;
            }
        } else {
            // We could be here if the item was stolen just before we read top_, we'll
            // adjust bottom_ below.
            assert( top - bottom == 1 );
        }
        // std::memory_order_relaxed used because we're not publishing any data. No
        // concurrent writes to bottom_ possible, it's always safe to write bottom_.
        bottom_.store( top, std::memory_order_relaxed );
        return item;
    }

    [[nodiscard]] T Steal() {
        // Note: A key component of this algorithm is that top_ is read before
        // bottom_ here (and observed as such in other threads).

        // std::memory_order_seq_cst is needed to guarantee ordering in pop(). Note
        // however that this is not a typical acquire operation (i.e. other thread's
        // writes of top_ don't publish data).
        auto top = top_.load( std::memory_order_seq_cst );

        // std::memory_order_acquire is needed because we're acquiring items
        // published in push(). std::memory_order_seq_cst is needed to guarantee
        // ordering in pop().
        if ( const auto bottom = bottom_.load( std::memory_order_seq_cst ); top >= bottom ) {
            // The queue is empty.
            return nullptr;
        }

        // The queue isn't empty.
        auto *array = array_.load( std::memory_order_acquire );
        const auto item = array->Get( top );
        if ( !top_.compare_exchange_strong( top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed ) ) {
            // Failure: the item we just tried to steal was pop()'ed under our feet,
            // simply discard it; nothing to do -- it's okay to try again.
            return nullptr;
        }
        // Success: we stole an item, just return it.
        return item;
    }

private:
    [[nodiscard]] Array<T> *Resize( Array<T> *array, const size_t bottom, const size_t top ) {
        auto *tmp = array->Resize( bottom, top );
        garbage_.push_back( array );
        std::swap( array, tmp );
        array_.store( array, std::memory_order_release );
        return array;
    }

#ifdef __cpp_lib_hardware_interference_size
    alignas( std::hardware_destructive_interference_size ) std::atomic<int> top_, bottom_;
#else
    std::atomic<int> top_, bottom_;
#endif
    std::atomic<Array<T> *> array_;
    std::vector<Array<T> *> garbage_;
};

}  // namespace poller