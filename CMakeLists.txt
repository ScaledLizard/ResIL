cmake_minimum_required(VERSION 2.8)


PROJECT(ImageLib)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

OPTION(BUILD_SHARED_LIBS "Build with shared (.DLL .SO) libraries." ON)


include(compat_flags)
# FIXME: config.h needs to be generated
#include(configh)

find_package(SDL)
if (SDL_FOUND)
	add_definitions(-DILUT_USE_SDL)
endif()
find_package(LCMS)
find_package(MNG)
find_package(JPEG)
find_package(PNG)
find_package(TIFF)
find_package(ZLIB)
find_package(OPENEXR)

SET(DEVIL_INCS

	${DEVIL_SOURCE_DIR}/include/IL/config.h
	${DEVIL_SOURCE_DIR}/include/IL/devil_internal_exports.h
	${DEVIL_SOURCE_DIR}/include/IL/il.h
	${DEVIL_SOURCE_DIR}/include/IL/il_wrap.h
	${DEVIL_SOURCE_DIR}/include/IL/ilu.h
	${DEVIL_SOURCE_DIR}/include/IL/ilu_region.h
	${DEVIL_SOURCE_DIR}/include/IL/ilut.h
)

SUBDIRS(
#	src-XTRA
	src-IL
	src-ILU
	src-ILUT
)

option(DEVIL_BUILD_EXAMPLES "build examples" OFF)
if (DEVIL_BUILD_EXAMPLES)
	add_subdirectory( examples)
endif()


# need to check for vc6 or vc2003 output dir
IF(BUILD_SHARED_LIBS)

	SET(DEVIL_LIB_DEST_DIR	${CMAKE_CURRENT_SOURCE_DIR}/lib/	)

ELSE(BUILD_SHARED_LIBS)

	SET(DEVIL_LIB_DEST_DIR	${CMAKE_CURRENT_SOURCE_DIR}/lib/	)

ENDIF(BUILD_SHARED_LIBS)



EXPORT_LIBRARY_DEPENDENCIES(CMakeDepends.txt)
