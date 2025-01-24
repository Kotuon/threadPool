
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
