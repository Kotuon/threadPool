
#ifndef FUNCTION_WRAPPER_HPP
#define FUNCTION_WRAPPER_HPP
#pragma once

#include <deque>
#include <memory>
#include <mutex>

class FunctionWrapper {
private:
    struct ImplBase {
        virtual void call() = 0;
        virtual ~ImplBase() {}
    };

    std::unique_ptr< ImplBase > Impl;

    template < typename F > struct ImplType : ImplBase {
        F Func;
        ImplType( F&& Func_ ) : Func( std::move( Func_ ) ) {}
        void call() { Func(); }
    };

public:
    template < typename F >
    FunctionWrapper( F&& Func_ )
        : Impl( new ImplType< F >( std::move( Func_ ) ) ) {}

    void operator()() {
        if ( Impl ) Impl->call();
    }

    FunctionWrapper() = default;

    FunctionWrapper( FunctionWrapper&& Other )
        : Impl( std::move( Other.Impl ) ) {}

    FunctionWrapper& operator=( FunctionWrapper&& Other ) {
        Impl = std::move( Other.Impl );
        return *this;
    }

    FunctionWrapper( const FunctionWrapper& ) = delete;
    FunctionWrapper( FunctionWrapper& ) = delete;
    FunctionWrapper& operator=( const FunctionWrapper& ) = delete;
};

#endif