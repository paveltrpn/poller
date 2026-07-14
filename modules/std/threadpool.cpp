
module;

#include <atomic>
#include <cassert>
#include <functional>
#include <new>
#include <optional>
#include <thread>
#include <vector>

export module poller_std:threadpool;

import :workstealingdeque;

namespace poller::pstd {

constexpr auto kCancelled = 1;
constexpr auto kInvoked = 1 << 1;

// Code below copied from https://github.com/dpuyda/scheduling licensed under the MIT License.
// Original code:
// https://github.com/dpuyda/scheduling

/**
 * \brief Represents a task in a task graph.
 *
 * A task graph is a collection of tasks and dependencies between them.
 * Dependencies between tasks define the order in which the tasks should be
 * executed.
 */
class Task {
public:
    /**
      * \brief Creates an empty task.
      *
      * Empty tasks can be used to define dependencies between task groups.
      */
    Task() = default;

    /**
      * \brief Creates a task.
      *
      * The signature of the function to execute should be equivalent to the
      * following:
      * \code{.cpp}
      * void func();
      * \endcode
      *
      * \param func The function to execute.
      */
    template <typename TaskType, typename = std::enable_if_t<std::convertible_to<TaskType, std::function<void()>>>>
    explicit Task( TaskType &&func )
        : func_{ std::forward<TaskType>( func ) } {}

    Task( const Task &other )
        : total_predecessors_{ other.total_predecessors_ }
        , func_{ other.func_ }
        , next_{ other.next_ } {
        remaining_predecessors_.store( other.remaining_predecessors_ );
        cancellation_flags_.store( other.cancellation_flags_ );
    }

    Task( Task &&other ) noexcept
        : total_predecessors_{ other.total_predecessors_ }
        , func_{ std::move( other.func_ ) }
        , next_{ std::move( other.next_ ) } {
        remaining_predecessors_.store( other.remaining_predecessors_ );
        cancellation_flags_.store( other.cancellation_flags_ );
    }

    Task &operator=( const Task &other ) {
        total_predecessors_ = other.total_predecessors_;
        remaining_predecessors_.store( other.remaining_predecessors_ );
        cancellation_flags_.store( other.cancellation_flags_ );
        func_ = other.func_;
        next_ = other.next_;
        return *this;
    }

    Task &operator=( Task &&other ) noexcept {
        total_predecessors_ = other.total_predecessors_;
        remaining_predecessors_.store( other.remaining_predecessors_ );
        cancellation_flags_.store( other.cancellation_flags_ );
        func_ = std::move( other.func_ );
        next_ = std::move( other.next_ );
        return *this;
    }

    ~Task() = default;

    /**
      * \brief Defines a task that should be executed before the current task.
      *
      * \param task A task that should be executed before the current task.
      */
    void Succeed( Task *task ) {
        task->next_.push_back( this );
        ++total_predecessors_;
        remaining_predecessors_.fetch_add( 1 );
    }

    /**
      * \brief Defines tasks that should be executed before the current task.
      *
      * \param task, tasks Tasks that should be executed before the current task.
      */
    template <typename... TasksType>
    void Succeed( Task *task, const TasksType &...tasks ) {
        task->next_.push_back( this );
        ++total_predecessors_;
        remaining_predecessors_.fetch_add( 1 );
        Succeed( tasks... );
    }

    /**
      * \brief Defines a task that should be executed after the current task.
      *
      * \param task A task that should be executed after the current task.
      */
    void Precede( Task *task ) {
        next_.push_back( task );
        ++task->total_predecessors_;
        task->remaining_predecessors_.fetch_add( 1 );
    }

    /**
      * \brief Defines tasks that should be executed after the current task.
      *
      * \param task, tasks Tasks that should be executed after the current task.
      */
    template <typename... TasksType>
    void Precede( Task *task, const TasksType &...tasks ) {
        next_.push_back( task );
        ++task->total_predecessors_;
        task->remaining_predecessors_.fetch_add( 1 );
        Precede( tasks... );
    }

    /**
      * \brief Cancels the task.
      *
      * Cancelling a task never fails. If `false` is returned, it means that the
      * task has been invoked earlier, or will be invoked at least once after the
      * cancellation. When a task is cancelled and will not be invoked anymore, its
      * successors also will not be invoked. Call `Reset` to undo cancellation.
      *
      * \see Reset
      *
      * \return `false` if the task has been invoked earlier or will be invoked at
      * least once after the cancellation, `true` otherwise.
      */
    bool Cancel() { return ( cancellation_flags_.fetch_or( kCancelled ) & kInvoked ) == 0; }

    /**
      * \brief Clears cancellation flags.
      *
      * Call `Reset` to undo task cancellation.
      *
      * \see Cancel
      */
    void Reset() { cancellation_flags_.store( 0 ); }

private:
    friend class ThreadPool;
    bool delete_{ false }, is_root_{ false };
    int total_predecessors_{ 0 };
    std::atomic<int> remaining_predecessors_{ 0 }, cancellation_flags_{ 0 };
    std::function<void()> func_;
    std::vector<Task *> next_;
};

/**
    * \brief A static thread pool that manages a specified number of background
    * threads and allows to execute tasks on these threads.
    *
    * The threads, managed by the thread pool, execute tasks in a work-stealing
    * manner.
    */
export class ThreadPool {
public:
    /**
      * \brief Creates a `ThreadPool` instance.
      *
      * When created, a `ThreadPool` instance creates a specified number of
      * threads that will be running in the background until the `ThreadPool`
      * instance is destroyed.
      *
      * \param threads_count The number of threads to create.
      */
    explicit ThreadPool( const unsigned threads_count = std::thread::hardware_concurrency() )
        : queues_count_{ threads_count + 1 }
        , queues_{ threads_count + 1 } {
        threads_.reserve( threads_count );
        for ( unsigned i = 0; i != threads_count; ++i ) {
            threads_.emplace_back( [this, i] -> void { run( i + 1 ); } );
        }
    }

    ThreadPool( const ThreadPool & ) = delete;
    ThreadPool( ThreadPool && ) = delete;
    auto operator=( const ThreadPool & ) -> ThreadPool & = delete;
    auto operator=( ThreadPool && ) -> ThreadPool & = delete;

    ~ThreadPool() noexcept {
        wait();
        stop_.test_and_set();
        tasks_count_ += queues_count_;
        tasks_count_.notify_all();
        for ( auto &thread : threads_ ) {
            thread.join();
        }
    }

    /**
      * \brief Submits a function that should be executed on a thread managed by
      * the thread pool.
      *
      * When submitted, the function is pushed into one of the thread pool task
      * queues. Eventually, the function will be popped from the queue and executed
      * on one of the threads managed by the thread pool. The order of function
      * execution is undetermined.
      *
      * The signature of the function should be equivalent to the following:
      * \code{.cpp}
      * void func();
      * \endcode
      *
      * \param func The function to execute.
      */
    template <typename FuncType, typename = std::enable_if_t<std::convertible_to<FuncType, std::function<void()>>>>
    auto submit( FuncType &&func ) -> void {
        auto *task = new Task( std::forward<FuncType>( func ) );
        task->delete_ = true;
        submit( task );
    }

    /**
      * \brief Submits a task that should be executed on a thread managed by the
      * thread pool.
      *
      * When submitted, the task is pushed into one of the thread pool task queues.
      * Eventually, the task will be popped from the queue and executed on one of
      * the threads managed by the thread pool. The order of task execution is
      * undetermined.
      *
      * \param task The task to execute.
      */
    auto submit( Task *task ) -> void {
        ++tasks_count_;
        queues_[index_].Push( task );
        tasks_count_.notify_one();
    }

    /**
      * \brief Submits a task graph that should be executed on threads managed by
      * the thread pool.
      *
      * A graph is a collection of tasks and dependencies between them. When
      * submitted, the tasks that do not have predecessors are pushed into the
      * thread pool task queues.
      *
      * \param tasks The tasks to execute.
      */
    template <typename TasksType>
    auto submit( TasksType &tasks ) -> void {
        for ( auto &task : tasks ) {
            task.is_root_ = task.total_predecessors_ == 0;
        }
        for ( auto &task : tasks ) {
            if ( task.is_root_ ) {
                Submit( &task );
            }
        }
    }

    /**
      * \brief Blocks the current thread and executes tasks from the task queues
      * until a specified predicate is satisfied.
      *
      * The signature of the predicate function should be equivalent to the
      * following:
      * \code{.cpp}
      * bool predicate();
      * \endcode
      *
      * \param predicate The predicate which returns `false` if the waiting should
      * be continued.
      */
    template <typename PredicateType>
    auto wait( const PredicateType &predicate ) -> void {
        while ( !predicate() ) {
            if ( auto *task = getTask() ) {
                execute( task );
            }
        }
    }

    /**
      * \brief Blocks the current thread until all task queues are empty.
      *
      * Other threads may push tasks into the task queues while the current thread
      * is blocked.
      */
    auto wait() const -> void {
        while ( const auto count = tasks_count_.load() ) {
            tasks_count_.wait( count );
        }
    }

private:
    auto run( const unsigned i ) -> void {
        index_ = i;
        for ( auto attempts = 0;; ) {
            if ( constexpr auto max_attempts = 100; ++attempts > max_attempts ) {
                tasks_count_.wait( 0 );
            }
            if ( auto *task = getTask() ) {
                execute( task );
                attempts = 0;
            } else if ( stop_.test() ) {
                return;
            }
        }
    }

    auto execute( Task *task ) -> void {
        for ( Task *next = nullptr; task; next = nullptr ) {
            task->remaining_predecessors_.store( task->total_predecessors_ );
            if ( task->cancellation_flags_.fetch_or( kInvoked ) & kCancelled ) {
                break;
            }
            if ( task->func_ ) {
                task->func_();
            }
            auto it = task->next_.begin();
            for ( ; it != task->next_.end(); ++it ) {
                if ( ( *it )->remaining_predecessors_.fetch_sub( 1 ) == 1 ) {
                    next = *it++;
                    break;
                }
            }
            for ( ; it != task->next_.end(); ++it ) {
                if ( ( *it )->remaining_predecessors_.fetch_sub( 1 ) == 1 ) {
                    submit( *it );
                }
            }
            if ( task->delete_ ) {
                delete task;
            }
            task = next;
        }
        if ( tasks_count_.fetch_sub( 1 ) == 1 ) {
            tasks_count_.notify_all();
        }
    }

    auto getTask() -> Task * {
        const auto i = index_;
        auto *task = queues_[i].Pop();
        if ( task ) {
            return task;
        }
        for ( unsigned j = 1; j != queues_count_; ++j ) {
            task = queues_[( i + j ) % queues_count_].Steal();
            if ( task ) {
                return task;
            }
        }
        return nullptr;
    }

    static thread_local unsigned index_;

    const unsigned queues_count_;

    std::atomic_flag stop_;
    std::atomic<unsigned> tasks_count_;

    std::vector<std::thread> threads_;
    std::vector<poller::WorkStealingDeque<Task *>> queues_;
};

inline thread_local unsigned ThreadPool::index_{ 0 };

}  // namespace poller::pstd
