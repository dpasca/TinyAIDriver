//==================================================================
/// TA_Brain.cpp
///
/// Created by Davide Pasca - 2023/04/28
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <random>
#include <numeric>

//==================================================================
template <typename T>
class SimpleNN
{
    static constexpr bool USE_XAVIER_INIT = true;
public:
    using Tensor = TensorT<T>;
private:
    struct Layer
    {
        Tensor Wei;
        Tensor Bia;
    };
    std::vector<Layer> mLs;
    size_t mMaxLenVecN {};

public:
    SimpleNN() = default;

    SimpleNN(const std::vector<size_t>& layerNs)
        : mLs(layerNs.size()-1)
    {
        for (size_t i=0; i < layerNs.size()-1; ++i)
        {
            mLs[i].Wei = Tensor(layerNs[i], layerNs[i+1]);
            mLs[i].Bia = Tensor(1, layerNs[i+1]);
        }

        mMaxLenVecN = *std::max_element(layerNs.begin(), layerNs.end());
    }

    // create from parameters
    SimpleNN(const Tensor& params, const std::vector<size_t>& layerNs)
        : SimpleNN(layerNs)
    {
        assert(params.size() == CalcNNSize(layerNs));

        const auto* ptr = params.data();
        for (auto& l : mLs)
        {
            l.Wei.LoadFromMem(ptr); ptr += l.Wei.size();
            l.Bia.LoadFromMem(ptr); ptr += l.Bia.size();
        }
    }

    // create from random seed
    SimpleNN(uint32_t seed, const std::vector<size_t>& layerNs)
        : SimpleNN(layerNs)
    {
        std::random_device rd;
        std::mt19937 gen( seed ? seed : rd() );
        if (USE_XAVIER_INIT)
        {
            // use Xavier initialization
            const T SQRT_2 = (T)std::sqrt(2.0);
            std::normal_distribution<T> dis((T)0.0, (T)1.0 / SQRT_2);

            // initialize weights and biases with random values
            for (auto& l : mLs)
            {
                l.Wei.ForEach([&](auto& x){ x = dis(gen); });
                l.Bia.ForEach([&](auto& x){ x = dis(gen); });
            }
        }
        else
        {
            // use random initialization
            std::uniform_real_distribution<T> dis((T)-1.0, (T)1.0);

            constexpr auto BIAS_SCALE = (T)0.1;

            // initialize weights and biases with random values
            for (auto& l : mLs)
            {
                l.Wei.ForEach([&](auto& x){ x = dis(gen); });
                l.Bia.ForEach([&](auto& x){ x = BIAS_SCALE * dis(gen); });
            }
        }
    }

    // flatten to a 1D tensor
    Tensor FlattenNN() const
    {
        Tensor flat(1, calcNNSize());
        auto* pData = flat.data();
        size_t pos = 0;
        for (const auto& l : mLs)
        {
            const auto* weiData = l.Wei.data();
            const auto* biaData = l.Bia.data();
            std::copy(weiData, weiData + l.Wei.size(), pData + pos); pos += l.Wei.size();
            std::copy(biaData, biaData + l.Bia.size(), pData + pos); pos += l.Bia.size();
        }
        return flat;
    }

    static size_t CalcNNSize(const std::vector<size_t>& layerNs)
    {
        size_t size = 0;
        for (size_t i=0; i < layerNs.size()-1; ++i)
            size += layerNs[i] * layerNs[i+1] + layerNs[i+1];
        return size;
    }
private:
    size_t calcNNSize() const
    {
        return std::accumulate(mLs.begin(), mLs.end(), (size_t)0,
            [](size_t sum, const Layer& l){ return sum + l.Wei.size() + l.Bia.size(); });
    }
public:
    void ForwardPass(Tensor& outs, const Tensor& ins) const
    {
        assert(ins.size()  == mLs[0].Wei.size_rows() &&
               outs.size() == mLs.back().Wei.size_cols());

        // define the activation function
        auto activ_vec = [](auto& v)
        {
            v.ForEach([](auto& x) {
                //x = T(1.0) / (T(1.0) + exp(-x)); // sigmoid
                //x = tanh(x); // tanh
                //x = std::max(T(0), x); // ReLU
                //x = std::max(T(0.01)*x, x); // Leaky ReLU
                x = x * T(0.5) * (T(1.0) + erf(x / sqrt(T(2.0)))); // GELU
            });
        };

        auto* pTempMem0 = (T*)alloca(mMaxLenVecN * sizeof(T));
        auto* pTempMem1 = (T*)alloca(mMaxLenVecN * sizeof(T));

        {
            Tensor tmp0(1, mLs[0].Wei.size_cols(), pTempMem0, false);
            Vec_mul_Mat(tmp0, ins, mLs[0].Wei);
            tmp0 += mLs[0].Bia;
            activ_vec(tmp0);
        }
        for (size_t i=1; i < mLs.size()-1; ++i)
        {
            const auto& l = mLs[i];
            Tensor tmp0(1, mLs[i-1].Wei.size_cols(), pTempMem0, false);
            Tensor tmp1(1, l.Wei.size_cols(), pTempMem1, false);
            Vec_mul_Mat(tmp1, tmp0, l.Wei);
            tmp1 += l.Bia;
            activ_vec(tmp1);
            std::swap(pTempMem0, pTempMem1);
        }
        {
            const auto& l = mLs.back();
            Tensor tmp0(1, mLs[mLs.size()-2].Wei.size_cols(), pTempMem0, false);
            Vec_mul_Mat(outs, tmp0, l.Wei);
            outs += l.Bia;
            activ_vec(outs);
        }
    }
};
