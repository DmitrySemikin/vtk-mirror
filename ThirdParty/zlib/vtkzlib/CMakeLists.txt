project(VTKZLIB)

include(CheckFunctionExists)
include(CheckIncludeFile)

#
# Check to see if VTK requires large file support
#
if(VTK_REQUIRE_LARGE_FILE_SUPPORT)
   add_definitions(-D_LARGEFILE64_SOURCE)
endif()

#
# Check for fseeko
#
check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
    add_definitions(-DNO_FSEEKO)
endif()

#
# Check for unistd.h
#
check_include_file(unistd.h Z_HAVE_UNISTD_H)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  set(ZLIB_DLL 1)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkzlibConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/vtkzlibConfig.h @ONLY)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#============================================================================
# zlib
#============================================================================

set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/vtkzlibConfig.h
    zconf.h
    zlib.h
)
set(ZLIB_PRIVATE_HDRS
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h
)
set(ZLIB_SRCS
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

set(ZLIB_DLL_SRCS
    zlib1.rc
)

vtk_add_library(
  vtkzlib
  ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_DLL_SRCS}
  ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtkzlibConfig.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkzlib
    COMPONENT Development)
endif()
