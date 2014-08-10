# - Find OPENEXR
# Find the native OPENEXR includes and library
# This module defines
#  OPENEXR_INCLUDE_DIR, where to find libmng.h, etc.
#  OPENEXR_LIBRARIES, the libraries needed to use OPENEXR.
#  OPENEXR_FOUND, If false, do not try to use OPENEXR.
# also defined, but not for general use are
#  OPENEXR_LIBRARY, where to find the OPENEXR library.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(OPENEXR_INCLUDE_DIR OpenEXRConfig.h
		PATH_SUFFIXES OpenEXR
	)

set(OPENEXR_NAMES ${OPENEXR_NAMES} IlmImf)
find_library(OPENEXR_LIBRARY NAMES ${OPENEXR_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set OPENEXR_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENEXR DEFAULT_MSG OPENEXR_LIBRARY OPENEXR_INCLUDE_DIR)

if(OPENEXR_FOUND)
  set(OPENEXR_LIBRARIES ${OPENEXR_LIBRARY})
endif()

# Deprecated declarations.
set (NATIVE_OPENEXR_INCLUDE_PATH ${OPENEXR_INCLUDE_DIR} )
if(OPENEXR_LIBRARY)
  get_filename_component (NATIVE_OPENEXR_LIB_PATH ${OPENEXR_LIBRARY} PATH)
endif()

mark_as_advanced(OPENEXR_LIBRARY OPENEXR_INCLUDE_DIR )
