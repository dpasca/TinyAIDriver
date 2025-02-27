//==================================================================
/// CS_Math.h
///
/// Created by Davide Pasca - 2023/04/28
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef CS_MATH_H
#define CS_MATH_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>

//#define CSM_MAT_COL_MAJOR

//==================================================================
template <typename T>
class CSM_VecT
{
    T*      mpData {};
    size_t  mSize {};
    bool    mOwnsData {true};

public:
    CSM_VecT()                           {}
    CSM_VecT(size_t size)                : mpData(new T[size]), mSize(size) {}
    CSM_VecT(T* pData, size_t size)      : mpData(pData), mSize(size), mOwnsData(false) {}
    CSM_VecT(const T* pData, size_t size): mpData((T*)pData), mSize(size), mOwnsData(false) {}
    CSM_VecT(size_t size, const T& fill) : CSM_VecT(size)
    {
        std::fill(mpData, mpData + mSize, fill);
    }
    CSM_VecT(size_t size, const T* pSrc) : CSM_VecT(size)
    {
        std::copy(pSrc, pSrc + mSize, mpData);
    }
    CSM_VecT(size_t size, const std::function<T(size_t)>& fun) : CSM_VecT(size)
    {
        for (size_t i = 0; i < mSize; ++i)
            mpData[i] = fun(i);
    }
    ~CSM_VecT()
    {
        if (mOwnsData) delete[] mpData;
    }

    // copy constructor
    CSM_VecT(const CSM_VecT& other) : CSM_VecT(other.mSize)
    {
        std::copy(other.mpData, other.mpData + mSize, mpData);
    }
    // copy assignment
    CSM_VecT& operator=(const CSM_VecT& other)
    {
        if (this == &other)
            return *this;

        if (mpData)
        {
            if (mOwnsData)
            {
                delete[] mpData;
                mpData = new T[other.mSize];
            }
            else
            {
                assert(mSize >= other.mSize);
            }
        }
        mSize = other.mSize;

        std::copy(other.mpData, other.mpData + mSize, mpData);

        return *this;
    }

    // move constructor
    CSM_VecT(CSM_VecT&& other)
    {
        *this = std::move(other);
    }

    // move assignment
    CSM_VecT& operator=(CSM_VecT&& other) noexcept
    {
        if (mpData && mOwnsData)
			delete[] mpData;
        mpData = other.mpData;
        mSize = other.mSize;
        other.mpData = nullptr;
        return *this;
    }
    // += operator
    CSM_VecT& operator+=(const CSM_VecT& other)
    {
        assert(mSize == other.mSize);
        for (size_t i=0; i < mSize; ++i)
            mpData[i] += other.mpData[i];
        return *this;
    }

          T& operator[](size_t i)       { assert( i < mSize ); return mpData[i]; }
    const T& operator[](size_t i) const { assert( i < mSize ); return mpData[i]; }

          T* data()       { return mpData; }
    const T* data() const { return mpData; }

    size_t size() const { return mSize; }

    void resize(size_t size)
    {
        if (size != mSize)
        {
            if (size < mSize)
            {
                if (mpData && mOwnsData)
                    delete[] mpData;
                mpData = new T[size];
            }
            mSize = size;
        }
    }

    void ZeroFill() { std::fill(mpData, mpData + mSize, T(0)); }

    void ForEach(const std::function<void(T&)> &func)
    {
        for (size_t i = 0; i < mSize; ++i)
            func(mpData[i]);
    }

    void LoadFromMem(const T* pSrc)
    {
        std::copy(pSrc, pSrc + mSize, mpData);
    }

    // iterators
    class iterator
    {
        T* mpData;
    public:
        iterator(T* pData) : mpData(pData) {}
        iterator& operator++() { ++mpData; return *this; }
        iterator operator++(int) { iterator tmp(*this); operator++(); return tmp; }
        bool operator==(const iterator& rhs) const { return mpData == rhs.mpData; }
        bool operator!=(const iterator& rhs) const { return mpData != rhs.mpData; }
        T& operator*() { return *mpData; }
    };

    class const_iterator
    {
        const T* mpData;
    public:
        const_iterator(const T* pData) : mpData(pData) {}
        const_iterator& operator++() { ++mpData; return *this; }
        const_iterator operator++(int) { const_iterator tmp(*this); operator++(); return tmp; }
        bool operator==(const const_iterator& rhs) const { return mpData == rhs.mpData; }
        bool operator!=(const const_iterator& rhs) const { return mpData != rhs.mpData; }
        const T& operator*() { return *mpData; }
    };
    iterator begin() { return iterator(mpData); }
    iterator end() { return iterator(mpData + mSize); }
    const_iterator begin() const { return const_iterator(mpData); }
    const_iterator end() const { return const_iterator(mpData + mSize); }
};

//==================================================================
template <typename T>
class CSM_MatT
{
    std::vector<T> mData;

    size_t         mRows {};
    size_t         mCols {};

public:
    CSM_MatT() {}
    CSM_MatT(size_t rows, size_t cols)
        : mData(rows * cols)
        , mRows(rows), mCols(cols)
    {}

    CSM_MatT(size_t rows, size_t cols, const T* pSrc)
        : mData(pSrc, pSrc + rows * cols)
        , mRows(rows), mCols(cols)
    {}

    // move constructor
    CSM_MatT(CSM_MatT&& other)
        : mData(std::move(other.mData)), mRows(other.mRows), mCols(other.mCols)
    {}

    // move assignment
    CSM_MatT& operator=(CSM_MatT&& other)
    {
        mData = std::move(other.mData);
        mRows = other.mRows;
        mCols = other.mCols;
        return *this;
    }

    T& operator()(size_t row, size_t col)
    {
        assert( row < mRows && col < mCols );
#ifdef CSM_MAT_COL_MAJOR
        return mData[col * mRows + row];
#else
        return mData[row * mCols + col];
#endif
    }

    const T& operator()(size_t row, size_t col) const
    {
        assert( row < mRows && col < mCols );
#ifdef CSM_MAT_COL_MAJOR
        return mData[col * mRows + row];
#else
        return mData[row * mCols + col];
#endif
    }

#ifdef CSM_MAT_COL_MAJOR
#else
          T* operator[](size_t row)       {assert(row < mRows); return &mData[row * mCols];}
    const T* operator[](size_t row) const {assert(row < mRows); return &mData[row * mCols];}
#endif
          auto* data()       { return mData.data(); }
    const auto* data() const { return mData.data(); }

    size_t size_rows() const { return mRows; }
    size_t size_cols() const { return mCols; }
    size_t size() const { return mData.size(); }

    void ForEach(std::function<void(T&)> func)
    {
        for (size_t i = 0; i < mData.size(); ++i)
            func(mData[i]);
    }

    void AppendToChromo(std::vector<T>& vec) const
    {
        vec.insert(vec.end(), mData.begin(), mData.end());
    }
    void LoadFromMem(const T* pSrc)
    {
        std::copy(pSrc, pSrc + mData.size(), mData.begin());
    }
};

inline auto CSM_Vec_mul_Mat = [](auto& resVec, const auto& vec, const auto& mat) -> auto&
{
    assert(resVec.size() == mat.size_cols());

    for (size_t i = 0; i < mat.size_cols(); ++i)
    {
        auto sum = decltype(vec[0])(0);
        for (size_t j = 0; j < mat.size_rows(); ++j)
            sum += vec[j] * mat(j, i);
        resVec[i] = sum;
    }
    return resVec;
};

//using CS_SCALAR = double;
using CS_SCALAR = float;

using CSM_Vec = CSM_VecT<CS_SCALAR>;
using CSM_Mat = CSM_MatT<CS_SCALAR>;

#endif

