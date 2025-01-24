
#ifndef STATIC_THREAD_POOL_HPP
#define STATIC_THREAD_POOL_HPP
#pragma once

#include <array>
#include <barrier>
#include <functional>
#include <future>
#include <thread>
#include <vector>

class FunctionWrapper;

class StaticThreadPool {
public:
    StaticThreadPool(
        size_t ThreadCount_ = std::thread::hardware_concurrency() );

    ~StaticThreadPool();

    template < class FunctionType, class... ArgTypes >
    void initialize( FunctionType&& Func, ArgTypes&&... Args ) {
        for ( size_t i = 0; i < ThreadCount; ++i ) {
            Tasks.push_back( std::bind( std::forward< FunctionType >( Func ),
                                        std::forward< ArgTypes >( Args )...,
                                        i ) );

            Threads.push_back(
                std::thread( &StaticThreadPool::workerThread, this, i ) );
        }
    }

    void runTask();

    const size_t getThreadCount() const;

private:
    void workerThread( size_t Id );

    std::vector< std::thread > Threads;
    std::vector< FunctionWrapper > Tasks;

    size_t ThreadCount = 32;

    std::barrier<> StartSync = std::barrier( ThreadCount + 1 );
    std::barrier<> FinishSync = std::barrier( ThreadCount + 1 );

    bool IsDone = false;
};

#endif
