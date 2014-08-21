# - Find LCMS
# Find the native LCMS includes and library
# This module defines
#  LCMS_INCLUDE_DIR, where to find liblcms.h, etc.
#  LCMS_LIBRARIES, the libraries needed to use LCMS.
#  LCMS_FOUND, If false, do not try to use LCMS.
# also defined, but not for general use are
#  LCMS_LIBRARY, where to find the LCMS library.

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

find_path(LCMS_INCLUDE_DIR lcms2.h)

set(LCMS_NAMES ${LCMS_NAMES} lcms)
find_library(LCMS_LIBRARY NAMES ${LCMS_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set LCMS_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LCMS DEFAULT_MSG LCMS_LIBRARY LCMS_INCLUDE_DIR)

if(LCMS_FOUND)
  set(LCMS_LIBRARIES ${LCMS_LIBRARY})
endif()

# Deprecated declarations.
set (NATIVE_LCMS_INCLUDE_PATH ${LCMS_INCLUDE_DIR} )
if(LCMS_LIBRARY)
  get_filename_component (NATIVE_LCMS_LIB_PATH ${LCMS_LIBRARY} PATH)
endif()

mark_as_advanced(LCMS_LIBRARY LCMS_INCLUDE_DIR )
