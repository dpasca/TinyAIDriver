//==================================================================
/// ImmGL.h
///
/// Created by Davide Pasca - 2022/05/26
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef IMMGL_H
#define IMMGL_H

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include "MathBase.h"

template <typename T> using IVec = std::vector<T>;

using IFloat2 = glm::vec2;
using IFloat3 = glm::vec3;
using IFloat4 = glm::vec4;
using IMat4   = glm::mat4;
using IColor4 = glm::vec4;

using IUInt = unsigned int;
using IStr  = std::string;

//==================================================================
template <typename T> static inline void resize_loose( IVec<T> &vec, size_t newSize )
{
    if ( newSize > vec.capacity() )
    {
        auto locmax = []( auto a, auto b ) { return a > b ? a : b; };
        vec.reserve( locmax( newSize, vec.capacity()/2*3 ) );
    }
    vec.resize( newSize );
}
//
template <typename T> static inline T *grow_vec( IVec<T> &vec, size_t growN )
{
    size_t n = vec.size();
    resize_loose<T>( vec, n + growN );
    return vec.data() + n;
}

//==================================================================
template <typename D, typename S>
static void ImmGL_MakeQuadOfTrigs( D *out, const S &v0, const S &v1, const S &v2, const S &v3 )
{
    out[0] = v0;
    out[1] = v1;
    out[2] = v2;
    out[3] = v3;
    out[4] = v2;
    out[5] = v1;
}

//==================================================================
class ShaderProg
{
public:
    IUInt   mProgramID  {};

    std::unordered_map<IStr,IUInt>  mLocs;

    ShaderProg( bool useTex );
    ~ShaderProg();

    const auto GetProgramID() const { return mProgramID; }

    void SetUniform( const char *pName, float v );
    void SetUniform( const char *pName, int v );
    void SetUniform( const char *pName, const IMat4 &m );
    void SetUniform( const char *pName, const IFloat3 &v );
    void SetUniform( const char *pName, const IFloat4 &v );
private:
    IUInt getLoc( const char *pName );
};

enum : size_t
{
    IMMGL_VT_POS,
    IMMGL_VT_COL,
    IMMGL_VT_TC0,
    IMMGL_VT_N
};

//==================================================================
class ImmGLList
{
public:
    IVec<IFloat3>   mVtxPos;
    IVec<IColor4>   mVtxCol;
    IVec<IFloat2>   mVtxTc0;
    IVec<uint32_t>  mIdx;

private:
    IUInt           mVAOs[ 1 << IMMGL_VT_N ]  {};
    IUInt           mVBOs[IMMGL_VT_N]         {};
    size_t          mCurVBOSizes[IMMGL_VT_N]  {};

    IUInt           mVAE        {};
    size_t          mCurVAESize {};

public:
    ImmGLList();
    ~ImmGLList();

    auto *AllocPos( size_t n ) { return grow_vec( mVtxPos, n ); }
    auto *AllocCol( size_t n ) { return grow_vec( mVtxCol, n ); }
    auto *AllocTc0( size_t n ) { return grow_vec( mVtxTc0, n ); }
    auto *AllocIdx( size_t n ) { return grow_vec( mIdx, n ); }

    void UpdateBuffers();

    void BindVAO() const;

    void CompileList();
    void DrawList( bool isTriangles );

    void ClearList()
    {
        mVtxPos.clear();
        mVtxCol.clear();
        mVtxTc0.clear();
        mIdx.clear();
    }
private:
    static size_t makeVAOIdx( size_t posi, size_t coli, size_t tc0i );
};

using ImmGLListPtr = std::unique_ptr<ImmGLList>;

//==================================================================
class ImmGL
{
    ImmGLList       mList;

    enum : int
    {
        BM_NONE,
        BM_ADD,
        BM_ALPHA,
    };
    int             mCurBlendMode = BM_NONE;

    enum : IUInt
    {
        FLG_LINES = 1 << 0,
        FLG_TEX   = 1 << 1,
        FLG_COL   = 1 << 2,
    };
    IUInt           mModeFlags = 0;
    IUInt           mCurTexID = 0;
    ShaderProg      *mpCurShaProg   {};
    IMat4           mCurMtxPS {1.f};

    IVec<std::unique_ptr<ShaderProg>>   moShaProgs;

public:
    ImmGL();
    ~ImmGL();

    void ResetStates();
    void FlushStdList();

    ImmGLListPtr NewList( const std::function<void (ImmGLList &)> &fn );
    void CallList( ImmGLList &lst, bool isTriangles=true );

    void SetBlendNone();
    void SetBlendAdd();
    void SetBlendAlpha();

    void SetTexture( IUInt texID );
    void SetNoTexture() { SetTexture( 0 ); }

    void SetMtxPS( const IMat4 &m );

    void DrawLine( const IFloat3 &p1, const IFloat3 &p2, const IColor4 &col );
    void DrawLine( const IFloat3 &p1, const IFloat3 &p2, const IColor4 &col1, const IColor4 &col2 );

    void DrawTri(const std::array<IFloat3,3> &poss, const std::array<IColor4,3> &cols);
    void DrawTri(const std::array<IFloat3,3> &poss, const IColor4 &col)
    {
        DrawTri( poss, std::array<IColor4,3>{col,col,col} );
    }

    void DrawQuad( const std::array<IFloat3,4> &poss, const std::array<IColor4,4> &cols );
    void DrawQuad( const std::array<IFloat3,4> &poss, const IColor4 &col );

    template <typename PT>
    void DrawRectFill( const PT &pos, const IFloat2 &siz, const std::array<IColor4,4> &cols );
    template <typename PT>
    void DrawRectFill( const PT &pos, const IFloat2 &siz, const IColor4 &col );

    void DrawRectFill( const IFloat4 &rc, const IColor4 &col )
    {
        DrawRectFill( IFloat2{rc[0],rc[1]}, IFloat2{rc[2],rc[3]}, col );
    }

private:
    std::array<IFloat3,4> makeRectVtxPos( const IFloat2 &pos, const IFloat2 &siz ) const
    {
        return { IFloat3{pos[0]+siz[0]*0, pos[1]+siz[1]*0, 0},
                 IFloat3{pos[0]+siz[0]*1, pos[1]+siz[1]*0, 0},
                 IFloat3{pos[0]+siz[0]*0, pos[1]+siz[1]*1, 0},
                 IFloat3{pos[0]+siz[0]*1, pos[1]+siz[1]*1, 0} };
    }
    std::array<IFloat3,4> makeRectVtxPos( const IFloat3 &pos, const IFloat2 &siz ) const
    {
        return { IFloat3{pos[0]+siz[0]*0, pos[1]+siz[1]*0, pos[2]},
                 IFloat3{pos[0]+siz[0]*1, pos[1]+siz[1]*0, pos[2]},
                 IFloat3{pos[0]+siz[0]*0, pos[1]+siz[1]*1, pos[2]},
                 IFloat3{pos[0]+siz[0]*1, pos[1]+siz[1]*1, pos[2]} };
    }

    void switchModeFlags( IUInt flags );
};

//==================================================================
inline void ImmGL::DrawLine( const IFloat3 &p1, const IFloat3 &p2, const IColor4 &col )
{
    DrawLine( p1, p2, col, col );
}

//==================================================================
inline void ImmGL::DrawLine(
        const IFloat3 &p1,
        const IFloat3 &p2,
        const IColor4 &col1,
        const IColor4 &col2 )
{
    switchModeFlags( FLG_LINES | FLG_COL );
    auto *pPos = mList.AllocPos( 2 );
    auto *pCol = mList.AllocCol( 2 );
    pPos[0] = { p1[0], p1[1], p1[2] };
    pPos[1] = { p2[0], p2[1], p2[2] };
    pCol[0] = col1;
    pCol[1] = col2;
}

//==================================================================
inline void ImmGL::DrawTri(const std::array<IFloat3,3> &poss, const std::array<IColor4,3> &cols)
{
    switchModeFlags( FLG_COL );
    auto *pPos = mList.AllocPos( 3 );
    auto *pCol = mList.AllocCol( 3 );
    pPos[0] = poss[0];
    pPos[1] = poss[1];
    pPos[2] = poss[2];
    pCol[0] = cols[0];
    pCol[1] = cols[1];
    pCol[2] = cols[2];
}

//==================================================================
inline void ImmGL::DrawQuad(
            const std::array<IFloat3,4> &poss,
            const std::array<IColor4,4> &cols )
{
    switchModeFlags( FLG_COL );
    auto *pPos = mList.AllocPos( 6 );
    auto *pCol = mList.AllocCol( 6 );
    ImmGL_MakeQuadOfTrigs( pPos, poss[0], poss[1], poss[2], poss[3] );
    ImmGL_MakeQuadOfTrigs( pCol, cols[0], cols[1], cols[2], cols[3] );
}

//==================================================================
inline void ImmGL::DrawQuad(
            const std::array<IFloat3,4> &poss,
            const IColor4 &col )
{
    switchModeFlags( FLG_COL );
    auto *pPos = mList.AllocPos( 6 );
    auto *pCol = mList.AllocCol( 6 );
    ImmGL_MakeQuadOfTrigs( pPos, poss[0], poss[1], poss[2], poss[3] );
    ImmGL_MakeQuadOfTrigs( pCol, col, col, col, col );
}

//==================================================================
template <typename PT>
inline void ImmGL::DrawRectFill(
            const PT &pos,
            const IFloat2 &siz,
            const std::array<IColor4,4> &cols )
{
    DrawQuad( makeRectVtxPos( pos, siz ), cols );
}

//==================================================================
template <typename PT>
inline void ImmGL::DrawRectFill(
            const PT &pos,
            const IFloat2 &siz,
            const IColor4 &col )
{
    DrawQuad( makeRectVtxPos( pos, siz ), col );
}

#endif

