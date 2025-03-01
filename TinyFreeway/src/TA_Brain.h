//==================================================================
/// TA_Brain.h
///
/// Created by Davide Pasca - 2023/04/28
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef TA_BRAIN_H
#define TA_BRAIN_H

#include <memory>
#include <vector>
#include "TA_Math.h"

template <typename T>
class SimpleNN;

//==================================================================
class Brain
{
    std::unique_ptr<SimpleNN<SCALAR>> moNN;
public:
    Brain(const Tensor& params, size_t insN, size_t outsN);
    Brain(uint32_t seed, size_t insN, size_t outsN);
    ~Brain();

    Tensor MakeBrainParams() const;

    void AnimateBrain(const Tensor& ins, Tensor& outs) const;
};

#endif
