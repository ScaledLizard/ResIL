# - Find MNG
# Find the native MNG includes and library
# This module defines
#  MNG_INCLUDE_DIR, where to find libmng.h, etc.
#  MNG_LIBRARIES, the libraries needed to use MNG.
#  MNG_FOUND, If false, do not try to use MNG.
# also defined, but not for general use are
#  MNG_LIBRARY, where to find the MNG library.

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

find_path(MNG_INCLUDE_DIR libmng.h)

set(MNG_NAMES ${MNG_NAMES} mng)
find_library(MNG_LIBRARY NAMES ${MNG_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set MNG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MNG DEFAULT_MSG MNG_LIBRARY MNG_INCLUDE_DIR)

if(MNG_FOUND)
  set(MNG_LIBRARIES ${MNG_LIBRARY})
endif()

# Deprecated declarations.
set (NATIVE_MNG_INCLUDE_PATH ${MNG_INCLUDE_DIR} )
if(MNG_LIBRARY)
  get_filename_component (NATIVE_MNG_LIB_PATH ${MNG_LIBRARY} PATH)
endif()

mark_as_advanced(MNG_LIBRARY MNG_INCLUDE_DIR )
