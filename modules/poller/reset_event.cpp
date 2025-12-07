module;

#include <coroutine>
#include <atomic>

export module poller:reset_event;

namespace poller {

export struct ResetEvent final {
public:
    explicit ResetEvent( bool initiallySet = false ) noexcept
        : m_state( initiallySet ? this : nullptr ) {
        /* noop*/
    }

    // No copying/moving
    ResetEvent( const ResetEvent& ) = delete;
    ResetEvent( ResetEvent&& ) = delete;
    auto operator=( const ResetEvent& ) -> ResetEvent& = delete;
    auto operator=( ResetEvent&& ) -> ResetEvent& = delete;

    ~ResetEvent() = default;

    struct awaiter {
        awaiter( const ResetEvent& event ) noexcept
            : m_event( event ) {
            /* noop*/
        }

        bool await_ready() const noexcept {
            //
            return m_event.is_set();
        }
        bool await_suspend(
            std::coroutine_handle<> awaitingCoroutine ) noexcept {
            // Special m_state value that indicates the event is in the 'set' state.
            const void* const setState = &m_event;

            // Stash the handle of the awaiting coroutine.
            m_awaitingCoroutine = awaitingCoroutine;

            // Try to atomically push this awaiter onto the front of the list.
            void* oldValue = m_event.m_state.load( std::memory_order_acquire );
            do {
                // Resume immediately if already in 'set' state.
                if ( oldValue == setState ) return false;

                // Update linked list to point at current head.
                m_next = static_cast<awaiter*>( oldValue );

                // Finally, try to swap the old list head, inserting this awaiter
                // as the new list head.
            } while ( !m_event.m_state.compare_exchange_weak(
                oldValue, this, std::memory_order_release,
                std::memory_order_acquire ) );

            // Successfully enqueued. Remain suspended.
            return true;
        }

        void await_resume() noexcept {
            //
        }

    private:
        friend struct ResetEvent;

        const ResetEvent& m_event;
        std::coroutine_handle<> m_awaitingCoroutine;
        awaiter* m_next;
    };

    awaiter operator co_await() const noexcept {
        //
        return awaiter{ *this };
    }

    bool is_set() const noexcept {
        //
        return m_state.load( std::memory_order_acquire ) == this;
    }

    void set() noexcept {
        // Needs to be 'release' so that subsequent 'co_await' has
        // visibility of our prior writes.
        // Needs to be 'acquire' so that we have visibility of prior
        // writes by awaiting coroutines.
        void* oldValue = m_state.exchange( this, std::memory_order_acq_rel );
        if ( oldValue != this ) {
            // Wasn't already in 'set' state.
            // Treat old value as head of a linked-list of waiters
            // which we have now acquired and need to resume.
            auto* waiters = static_cast<awaiter*>( oldValue );
            while ( waiters != nullptr ) {
                // Read m_next before resuming the coroutine as resuming
                // the coroutine will likely destroy the awaiter object.
                auto* next = waiters->m_next;
                waiters->m_awaitingCoroutine.resume();
                waiters = next;
            }
        }
    }

    void reset() noexcept {
        void* oldValue = this;
        m_state.compare_exchange_strong( oldValue, nullptr,
                                         std::memory_order_acquire );
    }

private:
    friend struct awaiter;

    // - 'this' => set state
    // - otherwise => not set, head of linked list of awaiter*.
    mutable std::atomic<void*> m_state;
};

}  // namespace poller
