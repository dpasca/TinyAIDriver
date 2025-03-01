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

#include "TA_Math.h"
#include "TA_Brain.h"

//==================================================================
static std::vector<size_t> makeLayerNs(size_t insN, size_t outsN)
{
    return std::vector<size_t>{
        insN,
        std::max((size_t)(insN * 1.25), outsN),
        std::max((size_t)(insN * 0.75), outsN),
        std::max((size_t)(insN * 0.25), outsN),
        outsN};
}

//==================================================================
Brain::Brain(const Tensor& params, size_t insN, size_t outsN)
{
    const auto layerNs = makeLayerNs(insN, outsN);
    mNN = SimpleNN<SCALAR>(params, layerNs);
}
//
Brain::Brain(uint32_t seed, size_t insN, size_t outsN)
{
    const auto layerNs = makeLayerNs(insN, outsN);
    mNN = SimpleNN<SCALAR>(seed, layerNs);
}

//
Brain::~Brain() = default;

//==================================================================
Tensor Brain::MakeBrainParams() const
{
    return mNN.FlattenNN();
}

//==================================================================
void Brain::AnimateBrain(const Tensor& ins, Tensor& outs) const
{
    mNN.ForwardPass(outs, ins);
}
