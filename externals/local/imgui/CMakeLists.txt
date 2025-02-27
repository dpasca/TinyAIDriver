project( imgui )

set( IMGUI_SRCDIR "../../../_externals/imgui/" )

file( GLOB SRCS "${IMGUI_SRCDIR}/*.cpp"
                "${IMGUI_SRCDIR}/*.h" )

include_directories( "${IMGUI_SRCDIR}/backends" )

# Regular SDL backend is always included
file( GLOB SRCS_IMPL "${IMGUI_SRCDIR}/backends/*_sdl2.*"
                     "${IMGUI_SRCDIR}/backends/*_sdlrenderer2.*" )
# OpenGL backend is optional
if (ENABLE_OPENGL)
    file( GLOB SRCS_IMPL_OGL "${IMGUI_SRCDIR}/backends/*_sdl2.*"
                             "${IMGUI_SRCDIR}/backends/*_opengl3.*"
                             "${IMGUI_SRCDIR}/backends/*_opengl2.*" )
endif()

file( GLOB SRCS_MISC "${IMGUI_SRCDIR}/misc/cpp/*.cpp"
                     "${IMGUI_SRCDIR}/misc/cpp/*.h" )

source_group( Lib  FILES ${SRCS} )
source_group( Impl FILES ${SRCS_IMPL} )
source_group( ImpOpenGL FILES ${SRCS_IMPL_OGL} )
source_group( Misc FILES ${SRCS_MISC} )

add_library( ${PROJECT_NAME} STATIC ${SRCS}
                                    ${SRCS_IMPL}
                                    ${SRCS_IMPL_OGL}
                                    ${SRCS_MISC} )
