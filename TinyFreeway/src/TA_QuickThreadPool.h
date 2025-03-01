//==================================================================
/// TA_QuickThreadPool.h
///
/// Created by Davide Pasca - 2025/03/01
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef TA_QUICKTHREADPOOL_H
#define TA_QUICKTHREADPOOL_H

#include <future>
#include <atomic>
#include <functional>
#include <vector>
#include <memory>

//==================================================================
// Very simple class to run jobs without worrying about available cores
class QuickThreadPool
{
    const size_t                   mThreadsN;
    std::vector<std::future<void>> mFutures;

public:
    QuickThreadPool( size_t threadsN ) : mThreadsN(threadsN)
    {
        mFutures.reserve( threadsN );
    }

    ~QuickThreadPool() { JoinTheads(); }

    void JoinTheads()
    {
        try {
            for (auto &f : mFutures)
                if (f.valid())
                    f.get();
        }
        catch(const std::exception& ex)
        {
            printf("ERROR: Uncaught Exception ! '%s'\n", ex.what());
            throw;
        }
    }

    void AddThread( std::function<void ()> fn )
    {
        // flush what's done
        mFutures.erase(
            std::remove_if(
                mFutures.begin(), mFutures.end(),
                [&]( const auto &a ){ return isFutureReady( a ); } ),
            mFutures.end() );

        // force wait if we're full
        while ( mFutures.size() >= mThreadsN )
        {
            mFutures[0].get();
            mFutures.erase( mFutures.begin() );
        }

        mFutures.push_back( std::async( std::launch::async, fn ) );
    }

private:
    static inline bool isFutureReady( const std::future<void> &f )
    {
        return f.wait_for( std::chrono::seconds(0) ) == std::future_status::ready;
    }
};

#endif
