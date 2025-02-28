//==================================================================
/// TA_Chromo.h
///
/// Created by Davide Pasca - 2023/04/28
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef TA_CHROMO_H
#define TA_CHROMO_H

#include <vector>
#include <string>
#include <sstream>
#include <utility>
#include <memory>
#include <cstring>
#include "TA_Math.h"

//==================================================================
class Chromo
{
public: // For now...
    std::vector<SCALAR> mChromoData;

public:
    Chromo CreateEmptyClone() const
    {
        Chromo chromo;
        chromo.mChromoData.resize(mChromoData.size());
        return chromo;
    }

    SCALAR* GetChromoData() { return mChromoData.data(); }
    const SCALAR* GetChromoData() const { return mChromoData.data(); }

    size_t GetSize() const { return mChromoData.size(); }
};

#endif
