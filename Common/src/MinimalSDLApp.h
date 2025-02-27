//==================================================================
/// MinimalSDLApp.h
///
/// Created by Davide Pasca - 2022/05/04
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef MINIMALSDLAPP_H
#define MINIMALSDLAPP_H

#include <array>
#include <string>
#include <functional>
#include "SDL.h"

//==================================================================
class MinimalSDLApp
{
    SDL_Window      *mpWindow   {};
    SDL_Surface     *mpSurface  {};
    SDL_Renderer    *mpRenderer {};
    SDL_GLContext   mpSDLGLContext {};

    size_t          mFrameCnt       {};
    double          mLastFrameTimeS {};

    bool            mUseSWRender    {};
    bool            mDisableVSync   {};
    size_t          mExitFrameN {};
    std::string     mSaveSShotPFName;

    bool            mShowMainUIWin  {true};

#ifdef ENABLE_OPENGL
    int             mUsingGLVersion_Major {};
    int             mUsingGLVersion_Minor {};
#endif
public:
    enum : int
    {
        FLAG_RESIZABLE  = 1,
        FLAG_OPENGL     = 2,
    };

    MinimalSDLApp( int argc, char *argv[], int w, int h, int flags=0 );
    ~MinimalSDLApp();

    bool BeginFrame();
    void EndFrame();

    std::array<int,2> GetDispSize() const;

    void DrawMainUIWin( const std::function<void ()> &fn );

    auto *GetRenderer() { return mpRenderer; }

    void SaveScreenshot( const std::string &pathFName );

private:
    void ctor_parseArgs( int argc, char *argv[] );
};

#ifdef ENABLE_IMGUI
# include "imgui.h"
# include "imgui_stdlib.h"
#endif

#endif

