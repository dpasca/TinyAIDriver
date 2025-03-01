//==================================================================
/// TA_Trainer.h
///
/// Created by Davide Pasca - 2023/04/28
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef TA_TRAINER
#define TA_TRAINER

#include <future>
#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include "TA_SimpleNN.h"
#include "TA_Train.h"

//==================================================================
static inline bool isFutureReady( const std::future<void> &f )
{
    return f.wait_for( std::chrono::seconds(0) ) == std::future_status::ready;
}

//==================================================================
class QuickThreadPool
{
    const size_t                   mTheadsN;
    std::vector<std::future<void>> mFutures;

public:
    QuickThreadPool( size_t threadsN )
        : mTheadsN(threadsN)
    {
        mFutures.reserve( threadsN );
    }

    ~QuickThreadPool()
    {
        JoinTheads();
    }

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
        while ( mFutures.size() >= mTheadsN )
        {
            mFutures[0].get();
            mFutures.erase( mFutures.begin() );
        }

        mFutures.push_back( std::async( std::launch::async, fn ) );
    }
};

//==================================================================
class Trainer
{
public:
    using CreateBrainFnT      = std::function<std::unique_ptr<SimpleNN>(const Tensor&, const std::vector<size_t>&)>;
    using EvalBrainT          = std::function<double (const SimpleNN&, std::atomic<bool>&)>;
    using OnEpochEndFnT       = std::function<std::vector<Tensor>(size_t,const Tensor*,const double*,size_t)>;

private:
    std::future<void>   mFuture;
    std::atomic<bool>   mShutdownReq {};
    size_t              mCurEpochN {};
    std::unique_ptr<Train> moTrain;

public:
    struct Params
    {
        size_t          maxEpochsN {};
        EvalBrainT      evalBrainFn;
    };
public:
    Trainer(const Params& par, std::unique_ptr<Train> &&oTrain)
        : moTrain( std::move(oTrain))
    {
        mFuture = std::async(std::launch::async, [this,par=par](){ ctor_execution(par); });
    }

    void LockViewBestPool(
            const std::function<void(
                const std::vector<Tensor>&,
                const std::vector<ParamsInfo>&
                )>& func)
    {
        moTrain->LockViewBestPool(func);
    }

private:
    void ctor_execution(const Params& par)
    {
        // get the starting population (i.e. random or from file)
        auto pool = moTrain->MakeStartPool();
        size_t popN = pool.size();

        for (size_t eidx=0; eidx < par.maxEpochsN && !mShutdownReq; ++eidx)
        {
            mCurEpochN = eidx;

            // fitnesses are the results of the execution
            std::vector<std::atomic<double>> fitnesses(popN);
            {
                // create a thread for each available core
                QuickThreadPool thpool( std::thread::hardware_concurrency() + 1 );

                // for each member of the population...
                for (size_t pidx=0; pidx < popN && !mShutdownReq; ++pidx)
                {
                    thpool.AddThread([this, &params=pool[pidx], &fitness=fitnesses[pidx], &par]()
                    {
                        // create and evaluate the net with the given parameters
                        fitness = par.evalBrainFn(*moTrain->CreateNetwork(params), mShutdownReq);
                    });
                }
            }

            // if we're shutting down, then exit before calling OnEpochEnd()
            if (mShutdownReq)
                break;

            // generate the new population
            std::vector<ParamsInfo> infos;
            infos.resize(popN);
            for (size_t pidx=0; pidx < popN; ++pidx)
            {
                auto& ci = infos[pidx];
                ci.ci_fitness = fitnesses[pidx];
                ci.ci_epochIdx = eidx;
                ci.ci_popIdx = pidx;
            }

            pool = moTrain->OnEpochEnd(eidx, pool.data(), infos.data(), popN);

            popN = pool.size();
        }
    }
public:
    auto& GetTrainerFuture() { return mFuture; }

    size_t GetCurEpochN() const { return mCurEpochN; }

    void ReqShutdown() { mShutdownReq = true; }
};

#endif
