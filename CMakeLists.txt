# This file is mostly the property of Kaiser and Altazimuth, horribly cobbled together by SaladBadger
#
# As part of the Kex Engine project, it is released by Kaiser under the GPL version 2

cmake_minimum_required(VERSION 3.11)
project(ChocolateDescent LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

cmake_policy(SET CMP0076 NEW)

set_property(GLOBAL PROPERTY KEX_ENGINE_SRCS "")
set_property(GLOBAL PROPERTY SOURCE_GROUPS "")

set(METADATA_BASE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" CACHE STRING "Base path for project")

# The following macros are provided by Kaiser and Altazimuth, from the Kex Engine project.
# ---------------------------------------------------------------------------------
#
macro(add_file)
    get_property(SRCS GLOBAL PROPERTY KEX_ENGINE_SRCS)
    foreach(_src ${ARGN})
        list(APPEND SRCS "${_src}")
    endforeach()
    set_property(GLOBAL PROPERTY KEX_ENGINE_SRCS ${SRCS})
endmacro()

macro(add_sources)
    set(_current_source_group)
    set(_expecting_file FALSE)
    set(_expecting_group_name FALSE)
    
    foreach(ARG ${ARGN})
        if(${ARG} STREQUAL "SOURCE_GROUP")
            set(_expecting_group_name TRUE)
        elseif(_expecting_group_name)
            set(_current_source_group ${ARG})
            string(REPLACE " " "_" _current_source_group_var ${_current_source_group})
            list(FIND SOURCE_GROUPS ${_current_source_group_var} _group_index)    
            
            if(_group_index EQUAL -1)
                list(APPEND SOURCE_GROUPS ${_current_source_group_var})
                set(SOURCE_GROUP_${_current_source_group_var})
            endif()
            
            set(_expecting_group_name FALSE)
            set(_expecting_file TRUE)
        elseif(_expecting_file)
            if(NOT _current_source_group)
                message(FATAL_ERROR "No source group name defined")
            endif()
            
            list(APPEND SOURCE_GROUP_${_current_source_group_var} ${ARG})
            
            if(NOT ${_current_source_group} STREQUAL "Root")
                source_group(${_current_source_group} FILES ${ARG})
            else()
                source_group("" FILES ${ARG})
            endif()
            
            if(EXISTS ${ARG})
                add_file(${ARG})
            endif()
        endif()
    endforeach()
endmacro()


find_package(SDL2)

add_subdirectory(2d)
add_subdirectory(3d)
add_subdirectory(cfile)
add_subdirectory(fix)
add_subdirectory(iff)
add_subdirectory(main_d1)
add_subdirectory(mem)
add_subdirectory(misc)
add_subdirectory(platform)
add_subdirectory(texmap)
add_subdirectory(ui)
add_subdirectory(vecmat)

get_property(PROJECT_FILES GLOBAL PROPERTY KEX_ENGINE_SRCS)
if (WIN32) #[ISB] there's probably a way a million times better to do this, but I'm having trouble wrapping my head around cmake variables ATM...............
    add_executable(ChocolateDescent WIN32 ${PROJECT_FILES}) #[ISB] sources added down the line... i guess?
else()
    add_executable(ChocolateDescent ${PROJECT_FILES}) #[ISB] sources added down the line... i guess?
endif()

#[ISB] if we got SDL2, set the USE_SDL definition
if (SDL2_FOUND)
    target_compile_definitions(ChocolateDescent PUBLIC USE_SDL)
endif()

include_directories(.) #When I asked about it, I was told once that "All cmake projects are bad." I think this is evidence of it.

if (SDL2_FOUND)
    target_link_libraries(ChocolateDescent PRIVATE
        ${SDL2_LIBRARIES}
    )

    target_include_directories(ChocolateDescent SYSTEM PRIVATE
        ${SDL2_INCLUDE_DIRS}
    )
endif()

if (SDL2_MAIN_FOUND)
    target_link_libraries(ChocolateDescent PRIVATE
        ${SDL2_MAIN_LIBRARIES}
    )
endif()
