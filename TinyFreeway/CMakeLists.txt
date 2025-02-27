project( TinyFreeway )

include_directories( . )
include_directories( src )

file( GLOB SRCS "src/*.cpp" )
file( GLOB INCS "src/*.h" )

source_group( Sources FILES ${SRCS} ${INCS} )

add_executable( ${PROJECT_NAME} ${SRCS} ${INCS} )

target_link_libraries( ${PROJECT_NAME} Common )

Copy_SDL_DLLs_to_RuntimeOut()
