//==================================================================
/// IncludeGL.h
///
/// Created by Davide Pasca - 2022/06/18
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef INCLUDEGL_H
#define INCLUDEGL_H

#ifdef ENABLE_OPENGL

#if defined(ANDROID) || defined(EMSCRIPTEN)
# include <EGL/egl.h>
# include <EGL/eglext.h>
#else
# include "GL/glew.h"
#endif

#endif

#endif

