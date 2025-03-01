//==================================================================
/// TA_TrainingManager.h
///
/// Created by Davide Pasca - 2025/03/01
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef TA_TRAININGMANAGER_H
#define TA_TRAININGMANAGER_H

#include <future>
#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include "TA_SimpleNN.h"
#include "TA_EvolutionEngine.h"
#include "TA_QuickThreadPool.h"

//==================================================================
class TrainingManager
{
private:
    std::future<void>   mFuture;
    std::atomic<bool>   mShutdownReq {};
    size_t              mCurEpochN {};
    EvolutionEngine     mEvEngine;

public:
    struct Params
    {
        std::vector<size_t> layerNs;
        size_t              maxEpochsN {};
        std::function<double (const SimpleNN&, std::atomic<bool>&)> calcFitnessFn;
    };
public:
    TrainingManager(const Params& par)
        : mEvEngine(par.layerNs)
    {
        // Create the main thread that will continue until reached maxEpochsN
        //  or until requested to shutdown via the atomic flag in calcFitnessFn
        mFuture = std::async(std::launch::async, [this,par=par](){ ctor_execution(par); });
    }

    ~TrainingManager()
    {
        mShutdownReq = true;
        if (mFuture.valid())
            mFuture.get();
    }

    void LockViewBestPool(
            const std::function<void(
                const std::vector<Tensor>&,
                const std::vector<ParamsInfo>&
                )>& func)
    {
        mEvEngine.LockViewBestPool(func);
    }

private:
    // Master execution thread that continuously runs the training
    //  one epoch at a time, with multiple threads to parallelize the
    //  fitness calculations of the population
    void ctor_execution(const Params& par)
    {
        // get the starting population (i.e. random or from file)
        auto pool = mEvEngine.CreateInitialPopulation();
        size_t popN = pool.size();

        // For each epoch...
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
                        fitness = par.calcFitnessFn(*mEvEngine.CreateNetwork(params), mShutdownReq);
                    });
                }
            }

            // if we're shutting down, then exit before calling CreateNewEvolution()
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

            // Ask the EvolutionEngine to generate the new population based on the results
            // of the last one
            pool = mEvEngine.CreateNewEvolution(eidx, pool.data(), infos.data(), popN);

            popN = pool.size();
        }
    }
public:
    auto& GetTrainerFuture() { return mFuture; }

    size_t GetCurEpochN() const { return mCurEpochN; }

    void ReqShutdown() { mShutdownReq = true; }
};

#endif
