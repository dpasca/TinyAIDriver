//==================================================================
/// TA_Train.h
///
/// Created by Davide Pasca - 2023/04/28
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef TA_TRAIN_H
#define TA_TRAIN_H

#include <sstream>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <random>
#include "TA_SimpleNN.h"

//==================================================================
static auto uniformCrossOver = [](auto& rng, const auto& a, const auto& b)
{
    auto res = a.CreateEmptyClone();
    auto* pRes = res.data();
    const auto* pA = a.data();
    const auto* pB = b.data();
    const auto n = a.size();

    std::uniform_real_distribution<double> uni(0.0, 1.0);
    for (size_t i=0; i < n; ++i)
        pRes[i] = uni(rng) < 0.5 ? pA[i] : pB[i];

    return res;
};

static auto calcMeanAndStddev = [](const auto& vec)
{
    float sum = 0.0f;
    float sum_squared = 0.0f;
    const auto* p = vec.data();
    const auto n = vec.size();

    for (size_t i=0; i < n; ++i)
    {
        const auto x = p[i];
        sum += x;
        sum_squared += x * x;
    }
    const auto mean = sum / n;
    const auto dcar = (sum_squared / n) - (mean * mean);
    const auto std_dev = sqrt(dcar);

    return std::make_pair(mean, std_dev);
};

static auto mutateNormalDist = [](auto& rng, const auto& vec, float rate)
{
    auto newVec = vec;
    const auto [mean, stddev] = calcMeanAndStddev(vec);
    auto* p = newVec.data();
    const auto n = newVec.size();

    std::normal_distribution<float> nor(mean, stddev);
    std::uniform_real_distribution<float> uni(0.0, 1.0);
    for (size_t i=0; i < n; ++i)
    {
        if (uni(rng) < rate)
            p[i] += (SCALAR)nor(rng);
    }
    return newVec;
};

static auto mutateScaled = [](auto& rng, const auto& vec, float rate)
{
    auto newVec = vec;
    double absSum = 0;

    auto* p = newVec.data();
    const auto n = newVec.size();
    for (size_t i=0; i < n; ++i)
        absSum += std::abs(p[i]);

    const auto avg = (SCALAR)(absSum / (double)n);
    const auto useSca = std::max( (SCALAR)1.0, avg );

    std::uniform_real_distribution<float> uni(0.0, 1.0);
    for (size_t i=0; i < n; ++i)
    {
        if (uni(rng) < rate)
            p[i] += (SCALAR)((uni(rng) * 2 - 1) * useSca);
    }
    return newVec;
};

//==================================================================
struct ParamsInfo
{
    double    ci_fitness {0.0};
    size_t    ci_epochIdx {0};
    size_t    ci_popIdx {0};

    std::string MakeStrID() const
    {
        std::stringstream ss;
        ss << "epoch:" << ci_epochIdx << ",idx:" << ci_popIdx;
        return ss.str();
    }
};

//==================================================================
class Train
{
    template <typename T> using function = std::function<T>;
    template <typename T> using vector = std::vector<T>;
    template <typename T> using unique_ptr = std::unique_ptr<T>;

    static constexpr size_t INIT_POP_N          = 100;
    static constexpr size_t TOP_FOR_SELECTION_N = 10;
    static constexpr size_t TOP_FOR_REPORT_N    = 10;

    std::vector<size_t>     mLayerNs;

    // best params list just for display
    std::mutex              mBestPoolMutex;
	std::vector<Tensor>     mBestPool;
    std::vector<ParamsInfo> mBestPInfos;

public:
    Train(const std::vector<size_t>& layerNs)
        : mLayerNs(layerNs)
    {
    }

    //==================================================================
    unique_ptr<SimpleNN> CreateNetwork(const Tensor &params)
    {
        return std::make_unique<SimpleNN>(params, mLayerNs);
    }

    //==================================================================
    // initial list of parameters
    vector<Tensor>  MakeStartPool()
    {
        std::vector<Tensor> pool;
        for (size_t i=0; i < INIT_POP_N; ++i)
        {
            // Generate a random network and store it as a flat tensor
            SimpleNN net((uint32_t)i, mLayerNs);
            pool.push_back( net.FlattenNN() );
        }
        return pool;
    }
    //==================================================================
    // when an epoch has ended
    vector<Tensor>  OnEpochEnd(
            size_t epochIdx,
            const Tensor* pPool,
            const ParamsInfo* pInfos,
            size_t n)
    {
        // sort by the cost
        std::vector<std::pair<const Tensor*, const ParamsInfo*>> pSorted;
        for (size_t i=0; i < n; ++i)
            pSorted.push_back({ pPool + i, pInfos + i });

        std::sort(pSorted.begin(), pSorted.end(), [](const auto& a, const auto& b)
        {
            return a.second->ci_fitness > b.second->ci_fitness;
        });

        // update the list of best params (with a lock... we're in a different thread)
        updateBestPool(pSorted);

        // random generator
        const auto seed = (unsigned int)epochIdx;
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        // mutation function
        auto mutateChromo = [&](const Tensor &params)
        {
            //return mutateScaled(rng, params, (SCALAR)0.2);
            return mutateNormalDist(rng, params, (SCALAR)0.1);
        };

        std::vector<Tensor> newPool;

        // elitism: keep top 1%
        //for (size_t i=0; i < std::max<size_t>(1, n/100); ++i)
        //    newPool.push_back( *pSorted[i].first );

        // breed the top N among each other with some mutations
        for (size_t i=0; i < TOP_FOR_SELECTION_N; ++i)
        {
            const auto& c_i = *pSorted[i].first;
            for (size_t j=i+1; j < (TOP_FOR_SELECTION_N-1); ++j)
            {
                const auto& c_j = *pSorted[j].first;
                newPool.push_back(              uniformCrossOver(rng, c_i, c_j) );
                newPool.push_back( mutateChromo(uniformCrossOver(rng, c_i, c_j)) );
                const auto& c_k = *pSorted[j+1].first;
                newPool.push_back(              uniformCrossOver(rng, c_i, c_k) );
                newPool.push_back( mutateChromo(uniformCrossOver(rng, c_i, c_k)) );
            }
        }

        return newPool;
    }

    //==================================================================
    void LockViewBestPool(
            const std::function<void(
                const std::vector<Tensor>&,
                const std::vector<ParamsInfo>&
                )>& func)
    {
        std::lock_guard<std::mutex> lock(mBestPoolMutex);
        func(mBestPool, mBestPInfos);
    }

private:
    //==================================================================
    void updateBestPool(
            const std::vector<std::pair<const Tensor*, const ParamsInfo*>>& pSorted)
    {
        std::lock_guard<std::mutex> lock(mBestPoolMutex);

        const auto n = std::min(TOP_FOR_REPORT_N, pSorted.size());

        // replace the best params list with the new best params
        mBestPool.clear();
        mBestPInfos.clear();
        for (size_t i=0; i < n; ++i)
        {
            mBestPool.push_back( *pSorted[i].first );
            mBestPInfos.push_back( *pSorted[i].second );
        }
    }
};

#endif
