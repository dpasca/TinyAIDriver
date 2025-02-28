//==================================================================
/// TA_Math.h
///
/// Created by Davide Pasca - 2023/04/28
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef TA_MATH_H
#define TA_MATH_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>

//==================================================================
template <typename T>
class TensorT
{
    T*             mpData {};
    bool           mOwnsData {true};
    size_t         mRows {};
    size_t         mCols {};

public:
    TensorT() {}
    TensorT(size_t rows, size_t cols)
        : mpData(new T[rows * cols])
        , mRows(rows), mCols(cols)
    {
        std::fill(mpData, mpData + mRows * mCols, T(0));
    }
    TensorT(size_t rows, size_t cols, T* pSrc, bool doCopy)
        : mpData(doCopy ? new T[rows * cols] : pSrc)
        , mOwnsData(doCopy)
        , mRows(rows), mCols(cols)
    {
        if (doCopy)
            std::copy(pSrc, pSrc + mRows * mCols, mpData);
    }
    TensorT(size_t rows, size_t cols, const T* pSrc, bool doCopy)
        : TensorT(rows, cols, (T*)pSrc, doCopy)
    {}

    // copy constructor
    TensorT(const TensorT& other)
        : mpData(new T[other.mRows * other.mCols])
        , mRows(other.mRows), mCols(other.mCols)
    {
        std::copy(other.mpData, other.mpData + mRows * mCols, mpData);
    }

    // move constructor
    TensorT(TensorT&& other) { *this = std::move(other); } // call move assignment

    ~TensorT()
    {
        if (mOwnsData)
            delete[] mpData;
    }

    // move assignment
    TensorT& operator=(TensorT&& other)
    {
        if (mpData && mOwnsData)
            delete[] mpData;
        mpData = other.mpData;
        mRows = other.mRows;
        mCols = other.mCols;
        other.mpData = nullptr;
        return *this;
    }

    // += operator
    TensorT& operator+=(const TensorT& other)
    {
        const auto n = mRows * mCols;
        assert(n == other.mRows * other.mCols);
        for (size_t i = 0; i < n; ++i)
            mpData[i] += other.mpData[i];
        return *this;
    }

    T& operator()(size_t row, size_t col)
    {
        assert( row < mRows && col < mCols );
        return mpData[row * mCols + col];
    }

    const T& operator()(size_t row, size_t col) const
    {
        assert( row < mRows && col < mCols );
        return mpData[row * mCols + col];
    }

          T* operator[](size_t row)       {assert(row < mRows); return &mpData[row * mCols];}
    const T* operator[](size_t row) const {assert(row < mRows); return &mpData[row * mCols];}
          auto* data()       { return mpData; }
    const auto* data() const { return mpData; }

    size_t size_rows() const { return mRows; }
    size_t size_cols() const { return mCols; }
    size_t size() const { return mRows * mCols; }

    void ForEach(std::function<void(T&)> func)
    {
        for (size_t i = 0; i < size(); ++i)
            func(mpData[i]);
    }

    void LoadFromMem(const T* pSrc)
    {
        std::copy(pSrc, pSrc + size(), mpData);
    }
};

inline auto Vec_mul_Mat = [](auto& resVec, const auto& vec, const auto& mat) -> auto&
{
    assert(resVec.size() == mat.size_cols());

    for (size_t i = 0; i < mat.size_cols(); ++i)
    {
        auto sum = decltype(vec(0,0))(0);
        for (size_t j = 0; j < mat.size_rows(); ++j)
            sum += vec(0,j) * mat(j, i);
        resVec(0,i) = sum;
    }
    return resVec;
};

//using SCALAR = double;
using SCALAR = float;

using Tensor = TensorT<SCALAR>;

#endif

