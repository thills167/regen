include(Utility)

find_include_path(GLEW NAMES GL/glew.h)

find_library_path(GLEW NAMES libGLEW.a libGLEW glew GLEW glew32 glew32s)

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLEW DEFAULT_MSG GLEW_LIBRARIES)

