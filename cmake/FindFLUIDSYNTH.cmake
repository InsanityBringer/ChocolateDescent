# [ISB] Adapted from the EE Cardboard project, from som and Altazimuth. 
# Copyright 2018 Benjamin Moir, et al.

# This code is licenced under the MIT License, which can be found at https://opensource.org/licenses/MIT

# [ISB] what the fuck am i doing?
# [ISB] no seriously the fuck am I doing?
# [ISB] is there a build system that's less criminal than CMake by chance? I'm eyeballing meson tbh...

find_path(FLUIDSYNTH_INCLUDE_DIRS
   NAMES
      fluidsynth.h
   HINTS
      $ENV{FLUIDSYNTHDIR}
   PATH_SUFFIXES
      include
)

find_library(FLUIDSYNTH_LIBRARIES
   NAMES
      fluidsynth #[ISB] TODO: Non-windows names....
   HINTS
      $ENV{FLUIDSYNTHDIR} #[ISB] TODO: Is there a standard name for this....
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(FLUIDSYNTH
   REQUIRED_VARS
      FLUIDSYNTH_LIBRARIES
      FLUIDSYNTH_INCLUDE_DIRS
)

set(FLUIDSYNTH_FOUND NO)

if(FLUIDSYNTH_LIBRARIES)
   set(FLUIDSYNTH_FOUND YES)
endif()
