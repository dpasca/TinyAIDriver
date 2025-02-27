//==================================================================
/// ImmGL.cpp
///
/// Created by Davide Pasca - 2022/05/26
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdexcept>
#include <GL/glew.h>
#include "ImmGL.h"

#undef  c_auto
#undef  NOT
#undef  __DSHORT_FILE__

#define c_auto const auto
#define NOT(X) (!(X))

//==================================================================
inline const char *DEX_SeekFilename( const char *pStr )
{
    return
        (strrchr(pStr,'/') ? strrchr(pStr,'/')+1 : \
            (strrchr(pStr,'\\') ? strrchr(pStr,'\\')+1 : pStr) );
}

//==================================================================
#define __DSHORT_FILE__ DEX_SeekFilename( __FILE__ )

//==================================================================
inline std::string DEX_MakeString( const char *pFmt, ... )
{
    constexpr size_t BUFF_LEN = 2048;

    char buff[ BUFF_LEN ];
    va_list args;
    va_start( args, pFmt );
    vsnprintf( buff, sizeof(buff), pFmt, args );
    va_end(args);

    buff[ BUFF_LEN-1 ] = 0;

    return buff;
}

# define DEX_RUNTIME_ERROR(_FMT_,...) \
    throw std::runtime_error( \
        DEX_MakeString( "[%s:%i] " _FMT_, __DSHORT_FILE__, __LINE__, ##__VA_ARGS__ ) )

//==================================================================
#if defined(DEBUG) || defined(_DEBUG)
# define CHECKGLERR CheckGLErr(__FILE__,__LINE__)
# define FLUSHGLERR FlushGLErr()
#else
# define CHECKGLERR
# define FLUSHGLERR
#endif

//==================================================================
static const char *getErrStr( GLenum err )
{
    switch ( err )
    {
    case GL_NO_ERROR:           return "GL_NO_ERROR";
    case GL_INVALID_ENUM:       return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:  return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:      return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:
        {
            static char buff[128] {};
            snprintf( buff, sizeof(buff), "#x%04x", err );
            return buff;
        }
    }
}

//==================================================================
static bool CheckGLErr( const char *pFileName, int line )
{
    bool didErr = false;
	GLenum err = glGetError();
	while (err != GL_NO_ERROR)
	{
        didErr = true;
        const char *pErrStr = getErrStr( err );

        if ( pErrStr )
            printf( "GL error: %s at %s : %i\n", pErrStr, pFileName, line );
        else
            printf( "Unknown error: %d 0x%x at %s : %i\n", err, err, pFileName, line );

		err = glGetError();
	}

    return didErr;
}

//==================================================================
static void FlushGLErr()
{
	while ( glGetError() != GL_NO_ERROR )
    {
    }
}

//==================================================================
static void check_shader_compilation( GLuint oid, bool isLink )
{
    GLint n {};
    if ( isLink )
        glGetProgramiv( oid, GL_LINK_STATUS, &n );
    else
        glGetShaderiv( oid, GL_COMPILE_STATUS, &n );

    if NOT( n )
    {
        if ( isLink )
            glGetProgramiv( oid, GL_INFO_LOG_LENGTH, &n );
        else
            glGetShaderiv( oid, GL_INFO_LOG_LENGTH, &n );

        IVec<GLchar>  info_log( n );

        if ( isLink )
            glGetProgramInfoLog( oid, n, &n, info_log.data() );
        else
            glGetShaderInfoLog( oid, n, &n, info_log.data() );

        DEX_RUNTIME_ERROR(
                "%s %s failed: %*s",
                isLink ? "Program" : "Shader",
                isLink ? "linking" : "compilation",
                n,
                info_log.data() );
    }
}

//==================================================================
ShaderProg::ShaderProg( bool useTex )
{
    CHECKGLERR;
static const IStr vtxSrouce = R"RAW(
// input attributes
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec4 a_col;
#ifdef USE_TEX
layout (location = 2) in vec2 a_tc0;
#endif

// out varyings
out vec4 v_col;
#ifdef USE_TEX
out vec2 v_tc0;
#endif

// vertx main
void main()
{
   v_col = a_col;
#ifdef USE_TEX
   v_tc0 = a_tc0;
#endif
   gl_Position = u_mtxPS * vec4( a_pos, 1.0 );
}
)RAW";

//---------------
static const IStr frgSource = R"RAW(
// input varyings
in vec4 v_col;
#ifdef USE_TEX
in vec2 v_tc0;
#endif

#ifdef USE_TEX
#endif

// out color
out vec4 o_col;

// fragment main
void main()
{
   o_col = v_col
#ifdef USE_TEX
            * texture( s_tex, v_tc0 )
#endif
            ;
}
)RAW";

    IStr header;
    
    // set the glsl version based on the opengl version
    {
        int ma = 0;
        int mi = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &ma);
        glGetIntegerv(GL_MINOR_VERSION, &mi);
        CHECKGLERR;
        header += "#version " + std::to_string(ma) + std::to_string(mi) + "0\n";
    }
    
    header += R"RAW(
uniform mat4x4  u_mtxPS;
uniform sampler2D s_tex;
)RAW";

    if ( useTex )
        header += "#define USE_TEX\n";

    auto makeShader = [&]( c_auto type, const IStr &src )
    {
        c_auto obj = glCreateShader( type );

        c_auto fullStr = header + src;
        const GLchar *ppsrcs[2] = { fullStr.c_str(), 0 };

        glShaderSource( obj, 1, &ppsrcs[0], nullptr ); CHECKGLERR;
        glCompileShader( obj ); CHECKGLERR;

        check_shader_compilation( obj, false );
        return obj;
    };

    c_auto shaderVtx = makeShader( GL_VERTEX_SHADER  , vtxSrouce );
    c_auto shaderFrg = makeShader( GL_FRAGMENT_SHADER, frgSource );

    mProgramID = glCreateProgram();
    CHECKGLERR;

    glAttachShader( mProgramID, shaderVtx );
    glAttachShader( mProgramID, shaderFrg );
    glLinkProgram( mProgramID );
    CHECKGLERR;

    check_shader_compilation( mProgramID, true );

    // always detach and delete shaders after a successful link
    glDetachShader( mProgramID, shaderVtx );
    glDetachShader( mProgramID, shaderFrg );
    glDeleteShader( shaderVtx );
    glDeleteShader( shaderFrg );
    CHECKGLERR;
}

//==================================================================
ShaderProg::~ShaderProg()
{
    if ( mProgramID )
        glDeleteProgram( mProgramID );
}

//==================================================================
IUInt ShaderProg::getLoc( const char *pName )
{
    auto it = mLocs.find( pName );
    if ( it == mLocs.end() )
        it = mLocs.insert({ pName, glGetUniformLocation( mProgramID, pName ) }).first;

    return it->second;
}

void ShaderProg::SetUniform( const char *pName, float v )
{
    glUniform1f( getLoc( pName ), v );
    CHECKGLERR;
}
void ShaderProg::SetUniform( const char *pName, int v )
{
    glUniform1i( getLoc( pName ), v );
    CHECKGLERR;
}
void ShaderProg::SetUniform( const char *pName, const IMat4 &m )
{
    glUniformMatrix4fv( getLoc( pName ), 1, false, (const float *)&m );
    CHECKGLERR;
}
void ShaderProg::SetUniform( const char *pName, const IFloat3 &v )
{
    glUniform3fv( getLoc( pName ), 1, (const float *)&v[0] );
    CHECKGLERR;
}
void ShaderProg::SetUniform( const char *pName, const IFloat4 &v )
{
    glUniform4fv( getLoc( pName ), 1, (const float *)&v[0] );
    CHECKGLERR;
}

//==================================================================
//==================================================================
size_t ImmGLList::makeVAOIdx( size_t posi, size_t coli, size_t tc0i )
{
    return (tc0i<<IMMGL_VT_TC0) | (coli<<IMMGL_VT_COL) | (posi<<IMMGL_VT_POS);
}

//==================================================================
inline auto updateBuff = []( auto &vec, c_auto type, c_auto &buff, auto &curSiz, bool bind )
{
    c_auto newSize = sizeof(vec[0]) * vec.size();
    if NOT( newSize )
        return;

    if ( bind )
    {
        glBindBuffer( type, buff );
        CHECKGLERR;
    }

    if ( newSize > curSiz ) // expand as necessary
    {
        curSiz = newSize;
        glBufferData( type, newSize, 0, GL_DYNAMIC_DRAW );
        CHECKGLERR;
    }
    glBufferSubData( type, 0, newSize, vec.data() );
    CHECKGLERR;
    if ( bind )
    {
        glBindBuffer( type, 0 );
        CHECKGLERR;
    }
};

//==================================================================
ImmGLList::ImmGLList()
{
    // make the VBOs
    glGenBuffers( IMMGL_VT_N, &mVBOs[0] );
    CHECKGLERR;

    for (size_t posi=1; posi < 2; ++posi)
    {
        for (size_t coli=0; coli < 2; ++coli)
        {
            for (size_t tc0i=0; tc0i < 2; ++tc0i)
            {
                IUInt vao {};
                glGenVertexArrays( 1, &vao );
                CHECKGLERR;
                glBindVertexArray( vao );
                CHECKGLERR;

                mVAOs[ makeVAOIdx( posi, coli, tc0i ) ] = vao;

                if ( posi ) glEnableVertexAttribArray( 0 );
                if ( coli ) glEnableVertexAttribArray( 1 );
                if ( tc0i ) glEnableVertexAttribArray( 2 );
                CHECKGLERR;

                auto vap = [this]( GLuint idx, GLuint cnt )
                {
                    glBindBuffer( GL_ARRAY_BUFFER, mVBOs[idx] );
                    glVertexAttribPointer( idx, cnt, GL_FLOAT, GL_FALSE, 0, nullptr );
                };

                if ( posi ) vap( 0, 3 );
                if ( coli ) vap( 1, 4 );
                if ( tc0i ) vap( 2, 2 );
                CHECKGLERR;

                glBindVertexArray( 0 );
                CHECKGLERR;
            }
        }
    }
    CHECKGLERR;

    // make the VAE
    glGenBuffers( 1, &mVAE );
    CHECKGLERR;
}

//==================================================================
ImmGLList::~ImmGLList()
{
    glDeleteBuffers( 1, &mVAE );

    for (c_auto v : mVAOs)
        if ( v )
            glDeleteVertexArrays( 1, &v );

    glDeleteBuffers( (GLsizei)IMMGL_VT_N, &mVBOs[0] );
}

//==================================================================
void ImmGLList::UpdateBuffers()
{
    updateBuff( mVtxPos,
        GL_ARRAY_BUFFER, mVBOs[IMMGL_VT_POS], mCurVBOSizes[IMMGL_VT_POS], true );
    updateBuff( mVtxCol,
        GL_ARRAY_BUFFER, mVBOs[IMMGL_VT_COL], mCurVBOSizes[IMMGL_VT_COL], true );
    updateBuff( mVtxTc0,
        GL_ARRAY_BUFFER, mVBOs[IMMGL_VT_TC0], mCurVBOSizes[IMMGL_VT_TC0], true );
}

//==================================================================
void ImmGLList::BindVAO() const
{
    c_auto posi = (size_t)1;
    c_auto coli = (size_t)!mVtxCol.empty();
    c_auto tc0i = (size_t)!mVtxTc0.empty();
    glBindVertexArray( mVAOs[ makeVAOIdx( posi, coli, tc0i ) ] );
    CHECKGLERR;
}

//==================================================================
void ImmGLList::CompileList()
{
    if ( mVtxPos.empty() )
        return;

    // update the vertex buffers
    UpdateBuffers();

    if NOT( mIdx.empty() )
    {
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mVAE );
        updateBuff( mIdx, GL_ELEMENT_ARRAY_BUFFER, mVAE, mCurVAESize, false );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        CHECKGLERR;
    }
}

//==================================================================
void ImmGLList::DrawList( bool isTriangles )
{
    c_auto primType = isTriangles ? GL_TRIANGLES : GL_LINES;

    BindVAO();

    if NOT( mIdx.empty() )
    {
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mVAE );
        glDrawElements( primType,  (GLsizei)mIdx.size(), GL_UNSIGNED_INT, nullptr );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    }
    else
    {
        glDrawArrays( primType, 0, (GLsizei)mVtxPos.size() );
    }

    glBindVertexArray( 0 );
    CHECKGLERR;
}

//==================================================================
//==================================================================
ImmGL::ImmGL()
{
    FLUSHGLERR;

    moShaProgs.push_back( std::make_unique<ShaderProg>( false ) );
    moShaProgs.push_back( std::make_unique<ShaderProg>( true ) );
}

//==================================================================
ImmGL::~ImmGL()
{
}

//==================================================================
void ImmGL::SetBlendNone()
{
    if ( mCurBlendMode == BM_NONE )
        return;

    mCurBlendMode = BM_NONE;
    FlushStdList();

    glDisable( GL_BLEND );
    CHECKGLERR;
}

//==================================================================
void ImmGL::SetBlendAdd()
{
    if ( mCurBlendMode == BM_ADD )
        return;

    mCurBlendMode = BM_ADD;
    FlushStdList();

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    CHECKGLERR;
}

//==================================================================
void ImmGL::SetBlendAlpha()
{
    if ( mCurBlendMode == BM_ALPHA )
        return;

    mCurBlendMode = BM_ALPHA;
    FlushStdList();

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    CHECKGLERR;
}

//==================================================================
void ImmGL::SetTexture( IUInt texID )
{
    if ( mCurTexID == texID )
        return;

    FlushStdList();
    mCurTexID = texID;
}

//==================================================================
void ImmGL::SetMtxPS( const IMat4 &m )
{
    if ( mCurMtxPS == m )
        return;

    FlushStdList();
    mCurMtxPS = m;
}

//==================================================================
ImmGLListPtr ImmGL::NewList( const std::function<void (ImmGLList &)> &fn )
{
    auto oList = std::make_unique<ImmGLList>();
    fn( *oList );
    oList->CompileList();
    return oList;
}

//==================================================================
void ImmGL::switchModeFlags( IUInt flags )
{
    if ( mModeFlags == flags )
        return;

    FlushStdList();
    mModeFlags = flags;
}

//==================================================================
void ImmGL::ResetStates()
{
    mCurBlendMode = BM_NONE;
    glDisable( GL_BLEND );
    CHECKGLERR;

    mModeFlags = 0;

    mCurTexID = (IUInt)0;
    glBindTexture( GL_TEXTURE_2D, mCurTexID );
    CHECKGLERR;

    //
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    CHECKGLERR;

    //
    mpCurShaProg = nullptr;
    glUseProgram( 0 );
    CHECKGLERR;

    //
    glBindVertexArray( 0 );
    CHECKGLERR;
}

//==================================================================
void ImmGL::FlushStdList()
{
    mList.CompileList();

    CallList( mList, !(mModeFlags & FLG_LINES) );

    mList.ClearList();
}

//==================================================================
void ImmGL::CallList( ImmGLList &lst, bool isTriangles )
{
    if ( lst.mVtxPos.empty() )
        return;

    //c_auto hasCol = !mList.mVtxCol.empty();
    c_auto hasTex = !mList.mVtxTc0.empty();

    CHECKGLERR;
    // set the texture
    if ( hasTex )
    {
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, mCurTexID );
    }
    else
    {
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
    CHECKGLERR;

    // set the program
    auto *pProg = moShaProgs[hasTex ? 1 : 0].get();
    if ( mpCurShaProg != pProg )
    {
        mpCurShaProg = pProg;
        glUseProgram( pProg->GetProgramID() ); CHECKGLERR;
    }

    // set the uniforms
    pProg->SetUniform( "u_mtxPS", mCurMtxPS );
    CHECKGLERR;

    lst.DrawList( isTriangles );

    // unbind everything
    //glUseProgram( 0 );
    CHECKGLERR;
}

//
#undef  c_auto
#undef  NOT
#undef  __DSHORT_FILE__

