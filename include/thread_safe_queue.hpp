
#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP
#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include "function_wrapper.hpp"

class WorkStealingQueue {
private:
    using DataType = FunctionWrapper;
    std::deque< DataType > TheQueue;
    mutable std::mutex TheMutex;

public:
    WorkStealingQueue() {}
    WorkStealingQueue( const WorkStealingQueue& Other ) = delete;
    WorkStealingQueue& operator=( const WorkStealingQueue& Other ) = delete;

    void push( DataType Data ) {
        std::lock_guard< std::mutex > Lock( TheMutex );
        TheQueue.push_front( std::move( Data ) );
    }

    bool empty() const {
        std::lock_guard< std::mutex > Lock( TheMutex );
        return TheQueue.empty();
    }

    bool tryPop( DataType& Res ) {
        std::lock_guard< std::mutex > Lock( TheMutex );
        if ( TheQueue.empty() ) return false;

        Res = std::move( TheQueue.front() );
        TheQueue.pop_front();
        return true;
    }

    bool trySteal( DataType& Res ) {
        std::lock_guard< std::mutex > Lock( TheMutex );
        if ( TheQueue.empty() ) return false;

        Res = std::move( TheQueue.back() );
        TheQueue.pop_back();
        return true;
    }
};

template < typename T > class ThreadSafeQueue {
private:
    struct Node {
        std::shared_ptr< T > Data;
        std::unique_ptr< Node > Next;
    };

    std::mutex HeadMutex;
    std::unique_ptr< Node > Head;
    std::mutex TailMutex;
    Node* Tail;
    std::condition_variable DataCond;

    Node* getTail() {
        std::lock_guard< std::mutex > TailLock( TailMutex );
        return Tail;
    }

    std::unique_ptr< Node > popHead() {
        std::unique_ptr< Node > OldHead = std::move( Head );
        Head = std::move( OldHead->Next );
        return OldHead;
    }

    std::unique_lock< std::mutex > waitForData() {
        std::unique_lock< std::mutex > HeadLock( HeadMutex );
        DataCond.wait( HeadLock, [&] { return Head.get() != getTail(); } );
        return std::move( HeadLock );
    }

    std::unique_ptr< Node > waitPopHead() {
        std::unique_lock< std::mutex > HeadLock( waitForData() );
        return popHead();
    }

    std::unique_ptr< Node > waitPopHead( T& Value ) {
        std::unique_lock< std::mutex > HeadLock( waitForData() );
        Value = std::move( *Head->Data );
        return popHead();
    }

    std::unique_ptr< Node > tryPopHead() {
        std::lock_guard< std::mutex > HeadLock( HeadMutex );
        if ( Head.get() == getTail() )
            return std::unique_ptr< Node >( nullptr );

        return popHead();
    }

    std::unique_ptr< Node > tryPopHead( T& Value ) {
        std::lock_guard< std::mutex > HeadLock( HeadMutex );
        if ( Head.get() == getTail() )
            return std::unique_ptr< Node >( nullptr );

        Value = std::move( *Head->Data );
        return popHead();
    }

public:
    ThreadSafeQueue() : Head( new Node ), Tail( Head.get() ) {}
    ThreadSafeQueue( const ThreadSafeQueue& Other ) = delete;
    ThreadSafeQueue& operator=( const ThreadSafeQueue& Other ) = delete;

    std::shared_ptr< T > tryPop() {
        std::unique_ptr< Node > OldHead = tryPopHead();
        return OldHead ? OldHead->data : std::shared_ptr< T >( nullptr );
    }

    bool tryPop( T& Value ) {
        std::unique_ptr< Node > const OldHead = tryPopHead( Value );
        return OldHead != nullptr;
    }

    std::shared_ptr< T > waitAndPop() {
        std::unique_ptr< Node > const OldHead = waitPopHead();
        return OldHead->Data;
    }

    void waitAndPop( T& Value ) {
        std::unique_ptr< Node > const OldHead = waitAndPop( Value );
    }

    void push( T NewValue ) {
        std::shared_ptr< T > NewData(
            std::make_shared< T >( std::move( NewValue ) ) );

        std::unique_ptr< Node > P( new Node );
        {
            std::lock_guard< std::mutex > TailLock( TailMutex );
            Tail->Data = NewData;
            Node* const NewTail = P.get();
            Tail->Next = std::move( P );
            Tail = NewTail;
        }
        DataCond.notify_one();
    }

    void empty() {
        std::lock_guard< std::mutex > HeadLock( HeadMutex );
        return ( Head.get() == getTail() );
    }
};

#endif
