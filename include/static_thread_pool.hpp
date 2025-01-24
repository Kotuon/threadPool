
#ifndef STATIC_THREAD_POOL_HPP
#define STATIC_THREAD_POOL_HPP
#pragma once

#include <array>
#include <barrier>
#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "function_wrapper.hpp"

#include "trace.hpp"

// std::array< unsigned, 32 > a = { 0 };
class StaticThreadPool {
public:
    StaticThreadPool(
        size_t ThreadCount_ = std::thread::hardware_concurrency() ) {
        ThreadCount = ThreadCount_;
    }

    ~StaticThreadPool() {
        if ( IsDone ) return;

        for ( auto& thd : Threads )
            thd.join();
    }

    template < class FunctionType, class... ArgTypes >
    void initialize( FunctionType&& Func, ArgTypes&&... Args ) {
        for ( size_t i = 0; i < ThreadCount; ++i ) {
            Tasks.push_back( std::bind( std::forward< FunctionType >( Func ), i,
                                        std::forward< ArgTypes >( Args )... ) );

            Threads.push_back(
                std::thread( &StaticThreadPool::workerThread, this, i ) );
        }
    }

    void runTask() {
        size_t j = 0;
        while ( !IsDone ) {
            StartSync.arrive_and_wait();

            ++j;
            if ( j > 2 ) {
                IsDone = true;
                Trace::message( "Set isDone to true." );
            }
            FinishSync.arrive_and_wait();
        }

        for ( auto& thd : Threads )
            thd.join();
    }

private:
    void workerThread( size_t Id ) {
        while ( !IsDone ) {
            StartSync.arrive_and_wait();
            Tasks[Id]();
            FinishSync.arrive_and_wait();
        }
    }

    std::vector< std::thread > Threads;
    std::vector< FunctionWrapper > Tasks;

    size_t ThreadCount = 32;

    std::barrier<> StartSync = std::barrier( ThreadCount + 1 );
    std::barrier<> FinishSync = std::barrier( ThreadCount + 1 );

    bool IsDone = false;
};

#endif
