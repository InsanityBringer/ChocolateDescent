# [ISB] From the EE Cardboard project, from som and Altazimuth. 
# Copyright 2018 Benjamin Moir, et al.

# This code is licenced under the MIT License, which can be found at https://opensource.org/licenses/MIT

find_path(SDL2_INCLUDE_DIRS
   NAMES
      SDL.h
   HINTS
      $ENV{SDL2DIR}
   PATH_SUFFIXES
      include
      include/SDL2
      SDL2
)

find_library(SDL2_LIBRARIES
   NAMES
      SDL2
   HINTS
      $ENV{SDL2DIR}
   PATH_SUFFIXES
      lib
      lib64
)

find_library(SDL2_MAIN_LIBRARIES
   NAMES
      SDL2main
   HINTS
      $ENV{SDL2DIR}
   PATH_SUFFIXES
      lib
      lib64
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(SDL2
   REQUIRED_VARS
      SDL2_LIBRARIES
      SDL2_INCLUDE_DIRS
)

set(SDL2_FOUND NO)
set(SDL2_MAIN_FOUND NO)

if(SDL2_LIBRARIES)
   set(SDL2_FOUND YES)
endif()

if(SDL2_MAIN_LIBRARIES)
   set(SDL2_MAIN_FOUND YES)
endif()