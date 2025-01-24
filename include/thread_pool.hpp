
#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

#include "function_wrapper.hpp"
#include "join_threads.hpp"
#include "thread_safe_queue.hpp"

#include "trace.hpp"

class ThreadPool {
private:
    using TaskType = FunctionWrapper;
    std::atomic_bool Done;
    ThreadSafeQueue< TaskType > PoolWorkQueue;
    std::vector< std::unique_ptr< WorkStealingQueue > > Queues;
    std::vector< std::thread > Threads;

    JoinThreads Joiner;

    inline static thread_local WorkStealingQueue* LocalWorkQueue;
    inline static thread_local size_t MyIndex;

    void workerThread( size_t MyIndex_ ) {
        MyIndex = MyIndex_;
        LocalWorkQueue = Queues[MyIndex].get();
        while ( !Done ) {
            runPendingTask();
        }
    }

    bool popTaskFromLocalQueue( TaskType& Task ) {
        return LocalWorkQueue && LocalWorkQueue->tryPop( Task );
    }

    bool popTaskFromPoolQueue( TaskType& Task ) {
        return PoolWorkQueue.tryPop( Task );
    }

    bool popTaskFromOtherThreadQueue( TaskType& Task ) {
        for ( size_t i = 0; i < Queues.size(); ++i ) {
            const size_t index = ( MyIndex + i + 1 ) % Queues.size();
            if ( Queues[index]->trySteal( Task ) ) return true;
        }

        return false;
    }

public:
    ThreadPool( const size_t ThreadCount = std::thread::hardware_concurrency() )
        : Done( false ), Joiner( Threads ) {
        try {
            for ( size_t i = 0; i < ThreadCount; ++i ) {
                Queues.push_back( std::unique_ptr< WorkStealingQueue >(
                    new WorkStealingQueue ) );

                Threads.push_back(
                    std::thread( &ThreadPool::workerThread, this, i ) );
            }
        } catch ( ... ) {
            Done = true;
            throw;
        }
    }

    ~ThreadPool() { Done = true; }

    template < class FunctionType, class... ArgTypes >
    std::future<
        typename std::invoke_result< FunctionType, ArgTypes... >::type >
    submit( FunctionType&& Func, ArgTypes&&... Args ) {
        using ResultType =
            typename std::invoke_result< FunctionType, ArgTypes... >::type;

        std::packaged_task< ResultType() > Task(
            std::bind( std::forward< FunctionType >( Func ),
                       std::forward< ArgTypes >( Args )... ) );

        std::future< ResultType > Result( Task.get_future() );

        if ( LocalWorkQueue )
            LocalWorkQueue->push( std::move( Task ) );
        else
            PoolWorkQueue.push( std::move( Task ) );

        return Result;
    }

    void runPendingTask() {
        TaskType Task;
        if ( popTaskFromLocalQueue( Task ) || popTaskFromPoolQueue( Task ) ||
             popTaskFromOtherThreadQueue( Task ) )
            Task();
        else
            std::this_thread::yield();
    }
};

#endif
