
#ifndef JOIN_THREADS_HPP
#define JOIN_THREADS_HPP
#pragma once

#include <thread>
#include <vector>

class JoinThreads {
private:
    std::vector< std::thread >& Threads;

public:
    explicit JoinThreads( std::vector< std::thread >& Threads_ )
        : Threads( Threads_ ) {}

    ~JoinThreads() {
        for ( size_t i = 0; i < Threads.size(); ++i ) {
            if ( Threads[i].joinable() ) Threads[i].join();
        }
    }
};

#endif
