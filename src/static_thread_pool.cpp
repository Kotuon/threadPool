
#include "static_thread_pool.hpp"

#include "function_wrapper.hpp"

StaticThreadPool::StaticThreadPool( size_t ThreadCount_ ) {
    ThreadCount = ThreadCount_;
}

StaticThreadPool::~StaticThreadPool() {
    if ( IsDone ) return;

    StartSync.arrive_and_wait();
    IsDone = true;
    FinishSync.arrive_and_wait();

    for ( auto& thd : Threads )
        thd.join();
}

template < class FunctionType, class... ArgTypes >
void StaticThreadPool::initialize( FunctionType&& Func, ArgTypes&&... Args ) {
    for ( size_t i = 0; i < ThreadCount; ++i ) {
        Tasks.push_back( std::bind( std::forward< FunctionType >( Func ),
                                    std::forward< ArgTypes >( Args )..., i ) );

        Threads.push_back(
            std::thread( &StaticThreadPool::workerThread, this, i ) );
    }
}

void StaticThreadPool::runTask() {
    StartSync.arrive_and_wait();
    FinishSync.arrive_and_wait();
}

void StaticThreadPool::workerThread( size_t Id ) {
    while ( !IsDone ) {
        StartSync.arrive_and_wait();
        Tasks[Id]();
        FinishSync.arrive_and_wait();
    }
}

const size_t StaticThreadPool::getThreadCount() const { return ThreadCount; }
