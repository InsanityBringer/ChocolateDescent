# [ISB] Adapted from the EE Cardboard project, from som and Altazimuth. 
# Copyright 2018 Benjamin Moir, et al.

# This code is licenced under the MIT License, which can be found at https://opensource.org/licenses/MIT

# [ISB] what the fuck am i doing?

find_path(OPENAL_INCLUDE_DIRS
   NAMES
      AL/al.h
   HINTS
      $ENV{OPENALDIR}
   PATH_SUFFIXES
      include
      include/AL
      AL
)

find_library(OPENAL_LIBRARIES
   NAMES
      OpenAL32 #[ISB] TODO: Non-windows names....
      openal
   HINTS
      $ENV{OPENALDIR} #[ISB] TODO: Is there a standard name for this....
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OPENAL
   REQUIRED_VARS
      OPENAL_LIBRARIES
      OPENAL_INCLUDE_DIRS
)

set(OPENAL_FOUND NO)

if(OPENAL_LIBRARIES)
   set(OPENAL_FOUND YES)
endif()
