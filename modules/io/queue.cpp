
module;

#include <atomic>
#include <cassert>
#include <vector>
#include <condition_variable>
#include <semaphore>

export module io:queue;

namespace tire::io {

// Single producer single consumer lock-free queue.
template <typename T>
struct spsc_lock_free_queue {
public:
    // capacity must be power of two to avoid using modulo operator
    // when calculating the index
    explicit spsc_lock_free_queue( size_t capacity )
        : capacity_( capacity )
        , buffer_( capacity ) {
        //assert((capacity & (capacity - 1)) == 0 && "capacity must be a
        //power of 2");
    }

    spsc_lock_free_queue( const spsc_lock_free_queue & ) = delete;
    spsc_lock_free_queue &operator=( const spsc_lock_free_queue & ) = delete;

    auto push( const T &item ) -> bool {
        std::size_t next_tail = ( tail_ + 1 ) & ( capacity_ - 1 );
        std::size_t tail = tail_.load( std::memory_order_relaxed );
        if ( next_tail != head_.load( std::memory_order_acquire ) ) {
            buffer_[tail] = item;
            tail_.store( next_tail, std::memory_order_release );
            return true;
        }
        return false;
    }

    auto pop( T &item ) -> bool {
        std::size_t head = head_.load( std::memory_order_relaxed );
        if ( head == tail_.load( std::memory_order_acquire ) ) {
            return false;
        }
        item = buffer_[head];
        head_.store( ( head + 1 ) & ( capacity_ - 1 ), std::memory_order_release );
        return true;
    }

private:
    const std::size_t capacity_;
    std::vector<T> buffer_;
    std::atomic<std::size_t> head_{ 0 };
    std::atomic<std::size_t> tail_{ 0 };
};

// Lock based queue.
template <typename T>
struct queue {
public:
    queue( std::size_t capacity )
        : capacity_{ capacity }
        , buffer_( capacity ) {}

    auto push( const T &item ) -> void {
        std::unique_lock<std::mutex> lock( mtx_ );
        not_full_.wait( lock, [this] { return !is_full(); } );
        buffer_[tail_] = item;
        tail_ = next( tail_ );
        lock.unlock();
        not_empty_.notify_one();
    }

    auto try_push( const T &item ) -> bool {
        std::unique_lock<std::mutex> lock( mtx_, std::try_to_lock );
        if ( !lock || is_full() ) {
            return false;
        }
        buffer_[tail_] = item;
        tail_ = next( tail_ );
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    auto pop( T &item ) -> void {
        std::unique_lock<std::mutex> lock( mtx_ );
        not_empty_.wait( lock, [this] { return !is_empty(); } );
        item = buffer_[head_];
        head_ = next( head_ );
        lock.unlock();
        not_full_.notify_one();
    }

    auto try_pop( T &item ) -> bool {
        std::unique_lock<std::mutex> lock( mtx_, std::try_to_lock );
        if ( !lock || is_empty() ) {
            return false;
        }
        item = buffer_[head_];
        head_ = next( head_ );
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

private:
    [[nodiscard]] auto next( std::size_t idx ) const noexcept -> size_t { return ( ( idx + 1 ) % capacity_ ); }

    [[nodiscard]] auto is_empty() const noexcept -> bool { return ( head_ == tail_ ); }

    [[nodiscard]] auto is_full() const noexcept -> bool { return ( next( tail_ ) == head_ ); }

private:
    std::mutex mtx_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::size_t head_{ 0 };
    std::size_t tail_{ 0 };
    std::size_t capacity_;
    std::vector<T> buffer_;
};

template <typename T>
class semaphore_queue {
public:
    semaphore_queue( std::size_t capacity )
        : sem_empty_( capacity )
        , sem_full_( 0 )
        , capacity_{ capacity }
        , buffer_( capacity ) {}

    auto push( const T &item ) -> void {
        sem_empty_.acquire();
        std::unique_lock<std::mutex> lock( mtx_ );
        buffer_[tail_] = item;
        tail_ = next( tail_ );
        lock.unlock();
        sem_full_.release();
    }

    auto try_push( const T &item ) -> bool {
        if ( !sem_empty_.try_acquire() ) {
            return false;
        }
        std::unique_lock<std::mutex> lock( mtx_ );
        buffer_[tail_] = item;
        tail_ = next( tail_ );
        lock.unlock();
        sem_full_.release();

        return true;
    }

    auto pop( T &item ) -> void {
        sem_full_.acquire();
        std::unique_lock<std::mutex> lock( mtx_ );
        item = buffer_[head_];
        head_ = next( head_ );
        lock.unlock();
        sem_empty_.release();
    }

    auto try_pop( T &item ) -> bool {
        if ( !sem_full_.try_acquire() ) {
            return false;
        }
        std::unique_lock<std::mutex> lock( mtx_ );
        item = buffer_[head_];
        head_ = next( head_ );
        lock.unlock();
        sem_empty_.release();
        return true;
    }

private:
    [[nodiscard]] auto next( std::size_t idx ) const noexcept -> size_t { return ( ( idx + 1 ) % capacity_ ); }

private:
    std::mutex mtx_;
    std::counting_semaphore<> sem_empty_;
    std::counting_semaphore<> sem_full_;
    std::size_t head_{ 0 };
    std::size_t tail_{ 0 };
    std::size_t capacity_;
    std::vector<T> buffer_;
};

}  // namespace tire::io
