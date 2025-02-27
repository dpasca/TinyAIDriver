//==================================================================
/// MinimalSDLApp.cpp
///
/// Created by Davide Pasca - 2022/05/27
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#include <string>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include "IncludeGL.h"
#ifdef ENABLE_IMGUI
# define IMGUI_DEFINE_MATH_OPERATORS
# include "imgui.h"
#  include "imgui_impl_sdl2.h"
# ifdef ENABLE_OPENGL
#  include "imgui_impl_opengl3.h"
#  include "imgui_impl_opengl2.h"
# endif
# include "imgui_impl_sdlrenderer2.h"
# include "imgui_internal.h"
#endif
#include "MinimalSDLApp.h"

static constexpr auto TARGET_FRAME_TIME_S = 1.0 / 60;

//==================================================================
inline double getSteadyTimeSecs()
{
    return
        (double)std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch() ).count() * 1e-6;
}

#ifdef _MSC_VER
inline int strcasecmp( const char *a, const char *b ) { return _stricmp( a, b ); }
#else
# include <strings.h> // for strcasecmp()
#endif

//==================================================================
void MinimalSDLApp::ctor_parseArgs( int argc, char *argv[] )
{
    auto printUsage = [&]( const char *pMsg )
    {
        printf( R"RAW(
Usage
  %s [options]

Options
  --help                       : This help
  --use_swrenderer             : Create a software rendering surface
  --autoexit_delay <frames>    : Automatically exit after a number of frames
  --autoexit_savesshot <fname> : Save a screenshot on automatic exit
)RAW", argv[0] );

        if ( pMsg )
            printf( "\n%s\n", pMsg );
    };

	for (int i=1; i < argc; ++i)
	{
        auto isparam = [&]( const auto &src ) { return !strcasecmp( argv[i], src ); };

        auto nextParam = [&]()
        {
			if ( ++i >= argc )
            {
				printUsage( "Missing parameters ?" );
                exit( 1 );
            }
            return argv[i];
        };

        if ( isparam("--help") )
        {
            printUsage(0);
            exit( 0 );
        }
        else if ( isparam("--use_swrenderer") )
        {
            mUseSWRender = true;
        }
        else if ( isparam("--autoexit_delay") )
        {
            mExitFrameN = (size_t)std::stoi( nextParam() );
            mDisableVSync = true; // also disables vsync
        }
        else if ( isparam("--autoexit_savesshot") )
        {
            mSaveSShotPFName = nextParam();
        }
        else if ( isparam("--disable_vsync") )
        {
            mDisableVSync = true;
        }
    }
}

//==================================================================
static std::string getFNameStem( const char *pPathFName )
{
    const auto str = std::filesystem::path( pPathFName ).filename().stem().u8string();
#if defined(__cpp_lib_char8_t)
	return std::string( str.begin(), str.end() );
#else
    return str;
#endif
}

//==================================================================
#ifdef ENABLE_OPENGL
static void setupOpenGLErrHandler()
{
    // setup OpenGL error handler
    auto glErrHandler = [](
            GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            const void *userParam )
    {
        if ( severity == GL_DEBUG_SEVERITY_NOTIFICATION )
            return;

        const char *pSource = "unknown";
        switch ( source )
        {
        case GL_DEBUG_SOURCE_API:             pSource = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   pSource = "window system"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: pSource = "shader compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     pSource = "third party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     pSource = "application"; break;
        case GL_DEBUG_SOURCE_OTHER:           pSource = "other"; break;
        }

        const char *pType = "unknown";
        switch ( type )
        {
        case GL_DEBUG_TYPE_ERROR:               pType = "error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: pType = "deprecated behavior"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  pType = "undefined behavior"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         pType = "portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         pType = "performance"; break;
        case GL_DEBUG_TYPE_MARKER:              pType = "marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          pType = "push group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           pType = "pop group"; break;
        case GL_DEBUG_TYPE_OTHER:               pType = "other"; break;
        }

        const char *pSeverity = "unknown";
        switch ( severity )
        {
        case GL_DEBUG_SEVERITY_HIGH:         pSeverity = "high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       pSeverity = "medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          pSeverity = "low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: pSeverity = "notification"; break;
        }

        SDL_LogError( SDL_LOG_CATEGORY_APPLICATION, "OpenGL error: %s, %s, %s, %s\n", pSource, pType, pSeverity, message );
    };

    if (GLEW_KHR_debug)
    {
        glEnable( GL_DEBUG_OUTPUT );
        glDebugMessageCallback( glErrHandler, nullptr );
    }
    else
    if (GLEW_ARB_debug_output)
    {
        glEnable( GL_DEBUG_OUTPUT );
        glDebugMessageCallbackARB( glErrHandler, nullptr );
    }
    else
    {
        SDL_LogWarn( SDL_LOG_CATEGORY_APPLICATION, "No OpenGL debug output available\n" );
    }
}

#endif

//==================================================================
inline auto exitErrSDL = []( const char *pMsg )
{
    SDL_LogError( SDL_LOG_CATEGORY_APPLICATION, "%s: %s\n", pMsg, SDL_GetError() );
    exit( 1 );
};

inline auto exitErr = [](const std::string& str)
{
    SDL_LogError( SDL_LOG_CATEGORY_APPLICATION, "%s", str.c_str() );
    exit( 1 );
};

inline void logInfo(const std::string& str)
{
    SDL_LogInfo( SDL_LOG_CATEGORY_APPLICATION, "%s", str.c_str() );
}

//==================================================================
MinimalSDLApp::MinimalSDLApp( int argc, char *argv[], int w, int h, int flags )
{
    ctor_parseArgs( argc, argv );

    // get the title from arg 0
    const auto title = getFNameStem( argv[0] );

    // enable logging
    SDL_LogSetPriority( SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO );

    // initialize SDL
    if ( SDL_Init(SDL_INIT_VIDEO) )
        exitErrSDL( "SDL_Init failed" );

#ifdef ENABLE_OPENGL
    if ( flags & FLAG_OPENGL )
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#  ifdef IS_GLES
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#  else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#  endif
# ifdef USE_WEBGL1
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
# else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
# endif
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        //SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );
    }
#endif

    // create the window
    const auto winFlags = (SDL_WindowFlags)(
#ifdef ENABLE_OPENGL
                        ((flags & FLAG_OPENGL)    ? SDL_WINDOW_OPENGL : 0) |
#endif
                        ((flags & FLAG_RESIZABLE) ? SDL_WINDOW_RESIZABLE  : 0) |
                        //SDL_WINDOW_ALLOW_HIGHDPI |
                        SDL_WINDOW_SHOWN );

    mpWindow = SDL_CreateWindow(
                        title.c_str(),
                        SDL_WINDOWPOS_UNDEFINED,
                        SDL_WINDOWPOS_UNDEFINED,
                        w, h, winFlags );
    if ( !mpWindow )
        exitErrSDL( "Window creation fail" );

#ifdef ENABLE_OPENGL
    if ( flags & FLAG_OPENGL )
    {
        // verify that the window has the OpenGL flag
        if (!(SDL_GetWindowFlags(mpWindow) & SDL_WINDOW_OPENGL))
            exitErr( "The window was created without OpenGL flag" );

# ifdef DEBUG
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
# endif
        mpSDLGLContext = SDL_GL_CreateContext( mpWindow );

        if ( GLEW_OK != glewInit() )
            exitErr( "Failed to initialize GLEW" );

        SDL_GL_MakeCurrent( mpWindow, mpSDLGLContext );

        glGetIntegerv( GL_MAJOR_VERSION, &mUsingGLVersion_Major );
        glGetIntegerv( GL_MINOR_VERSION, &mUsingGLVersion_Minor );

        logInfo(fmt::format("Using OpenGL {}.{}", mUsingGLVersion_Major, mUsingGLVersion_Minor));

        SDL_GL_SetSwapInterval( mDisableVSync ? 0 : 1 );
    }
#endif

    if ( mUseSWRender )
    {
        // create the surface
        mpSurface = SDL_GetWindowSurface( mpWindow );
        if ( !mpSurface )
            exitErrSDL( "SDL_GetWindowSurface failed" );

        // create the renderer
        mpRenderer = SDL_CreateSoftwareRenderer( mpSurface );

        if ( !mpRenderer )
            exitErrSDL( "SDL_CreateSoftwareRenderer failed" );
    }
    // create a hardware renderer, but only if we aren't using OpenGL
    else
    if ( !(flags & FLAG_OPENGL) )
    {
        mpRenderer = SDL_CreateRenderer( mpWindow, -1, 0
                        | SDL_RENDERER_ACCELERATED
                        | (mDisableVSync ? 0 : SDL_RENDERER_PRESENTVSYNC) );

        if ( !mpRenderer )
            exitErrSDL( "SDL_CreateRenderer failed" );
    }

#ifdef ENABLE_IMGUI
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

# ifdef ENABLE_OPENGL
    if ( mpSDLGLContext )
    {
        ImGui_ImplSDL2_InitForOpenGL( mpWindow, mpSDLGLContext );
        if ( mUsingGLVersion_Major >= 3 )
        {
            const char *pGLSLVer = mUsingGLVersion_Major > 3 || mUsingGLVersion_Minor >= 2
                                        ? "#version 150"
                                        : "#version 130";
            ImGui_ImplOpenGL3_Init( pGLSLVer );
        }
        else
        {
            ImGui_ImplOpenGL2_Init();
        }
    }
    else
# endif
    if ( mpRenderer )
    {
        ImGui_ImplSDL2_InitForSDLRenderer( mpWindow, mpRenderer );
        ImGui_ImplSDLRenderer2_Init( mpRenderer );
    }
#endif

#ifdef ENABLE_OPENGL
    setupOpenGLErrHandler();
#endif
}

//==================================================================
MinimalSDLApp::~MinimalSDLApp()
{
    SDL_Quit();
}

//==================================================================
bool MinimalSDLApp::BeginFrame()
{
#ifdef ENABLE_OPENGL
    if ( mpSDLGLContext )
        SDL_GL_MakeCurrent( mpWindow, mpSDLGLContext );
#endif

    SDL_Event e;
    while ( SDL_PollEvent(&e) )
    {
        if ( e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F2 )
            mShowMainUIWin = !mShowMainUIWin;

#ifdef ENABLE_IMGUI
        ImGui_ImplSDL2_ProcessEvent( &e );
#endif
        if (e.type == SDL_QUIT)
            return false;

        if ( e.type == SDL_KEYDOWN  &&  e.key.keysym.sym == SDLK_ESCAPE )
            return false;
    }

    // see if we need to exit because of the established timeout
    if ( mExitFrameN && mFrameCnt >= mExitFrameN )
    {
        printf( "Automatic exit\n" );
        if ( !mSaveSShotPFName.empty() )
        {
            printf( "Saving screenshot %s\n", mSaveSShotPFName.c_str() );
            SaveScreenshot( mSaveSShotPFName );
        }

        return false;
    }

#ifdef ENABLE_IMGUI
# ifdef ENABLE_OPENGL
    if ( mpSDLGLContext )
    {
        if ( mUsingGLVersion_Major >= 3 )
            ImGui_ImplOpenGL3_NewFrame();
        else
            ImGui_ImplOpenGL2_NewFrame();
    }
    else
#endif
    {
        ImGui_ImplSDLRenderer2_NewFrame();
    }

    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
#endif

    return true;
}

//==================================================================
void MinimalSDLApp::EndFrame()
{
#ifdef ENABLE_IMGUI
    ImGui::Render();
# ifdef ENABLE_OPENGL
    if ( mpSDLGLContext )
    {
        glViewport( 0, 0, GetDispSize()[0], GetDispSize()[1] );

        if ( mUsingGLVersion_Major >= 3 )
            ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
        else
            ImGui_ImplOpenGL2_RenderDrawData( ImGui::GetDrawData() );
    }
    else
# endif
    {
        ImGui_ImplSDLRenderer2_RenderDrawData( ImGui::GetDrawData(), mpRenderer );
    }
#endif

    // rudimentary frame sync, only if we're not in auto-exit mode
    if ( !mDisableVSync )
    {
        const auto curTimeS = getSteadyTimeSecs();

        if ( mUseSWRender )
        {
            const auto elapsedS = curTimeS - mLastFrameTimeS;

            if ( elapsedS < TARGET_FRAME_TIME_S )
                SDL_Delay( (uint32_t)((TARGET_FRAME_TIME_S - elapsedS) * 1000) );
        }

        mLastFrameTimeS = curTimeS;
    }

    // check if there is any error from SDL, and just quit if there is
    if (const char *pErr = SDL_GetError(); pErr[0])
        exitErr(fmt::format("SDL error: {}", pErr));

    if ( mUseSWRender )
        SDL_UpdateWindowSurface( mpWindow );
    else
    {
#ifdef ENABLE_OPENGL
        if ( mpSDLGLContext )
            SDL_GL_SwapWindow( mpWindow );
        else
#endif
        if ( mpRenderer )
            SDL_RenderPresent( mpRenderer );
    }

    // check if there is any error from SDL, and just quit if there is
    if (const char *pErr = SDL_GetError(); pErr[0])
        exitErr(fmt::format("SDL error: {}", pErr));

    mFrameCnt += 1;
}

//==================================================================
std::array<int,2> MinimalSDLApp::GetDispSize() const
{
    int w {};
    int h {};
    // get the window size
    SDL_GetWindowSize( mpWindow, &w, &h );
    return {w, h};
}

//==================================================================
void MinimalSDLApp::DrawMainUIWin( const std::function<void ()> &fn )
{
#ifdef ENABLE_IMGUI
    if ( !mShowMainUIWin )
        return;

    ImGui::SetNextWindowPos( {4,4}, ImGuiCond_Once );
    if ( ImGui::Begin( "Main Win (F2: Show/hide)",
                            &mShowMainUIWin, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        fn();
    }
    ImGui::End();
#endif
}

//==================================================================
void MinimalSDLApp::SaveScreenshot( const std::string &pathFName )
{
    const auto fmt = SDL_PIXELFORMAT_RGB888;

    const auto [w, h] = GetDispSize();
    if (!w || !h)
        return;

    auto *pTmpSurf = SDL_CreateRGBSurfaceWithFormat( 0, w, h, 24, fmt );

#ifdef ENABLE_OPENGL
    if ( mpSDLGLContext )
    {
        //glReadBuffer( GL_FRONT );
        glReadPixels( 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pTmpSurf->pixels );
        auto *p = (uint8_t *)pTmpSurf->pixels;
        for (int y=0; y < (h/2); ++y)
        {
            auto *p1 = p +    y    * (size_t)pTmpSurf->pitch;
            auto *p2 = p + (h-y-1) * (size_t)pTmpSurf->pitch;
            for (int x=0; x < (w*4); ++x)
            {
                std::swap( p1[x+0], p2[x+0] );
                std::swap( p1[x+1], p2[x+1] );
                std::swap( p1[x+2], p2[x+2] );
            }
        }
        for (int y=0; y < h; ++y)
        {
            auto *p1 = p + y * (size_t)pTmpSurf->pitch;
            for (int x=0; x < w; ++x)
                std::swap( p1[x*4+0], p1[x*4+2] );
        }
    }
    else
#endif
    if ( mpRenderer )
    {
        SDL_RenderReadPixels( mpRenderer, nullptr, fmt, pTmpSurf->pixels, pTmpSurf->pitch );
    }

    SDL_SaveBMP( pTmpSurf, pathFName.c_str() );
    SDL_FreeSurface( pTmpSurf );
}
