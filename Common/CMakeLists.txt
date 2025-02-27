project( Common )

include_directories( src )
# include glm, this is all that it's needed
include_directories( ../_externals/glm )

file( GLOB SRCS "src/*.cpp" )
file( GLOB INCS "src/*.h" )

# adding this to the exe is only for intellisense, not to compile
file( GLOB INCS_GLM "../_externals/glm/glm/*.hpp" )

source_group( Sources FILES ${SRCS} ${INCS} )
source_group( glm FILES ${INCS_GLM} )

add_library( ${PROJECT_NAME} ${SRCS} ${INCS} ${INCS_GLM} )

target_link_libraries( ${PROJECT_NAME}
    ${SDL_LIBRARIES}
    ${SDL_LIBRARIES_EXTRA}
    ${GFX_LINK_LIBS}
    ${PLATFORM_LINK_LIBS}
    ${IMGUI_LIBRARIES}
    fmt::fmt
    )

