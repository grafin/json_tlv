cmake_minimum_required(VERSION 2.8.12)

project(nxlog_json_packer C)

set(CMAKE_COLOR_MAKEFILE OFF)

include(FindPkgConfig)
include(GNUInstallDirs)

set(CMAKE_C_FLAGS_SANITIZER "-g -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer" CACHE STRING
    "Flags used by the C compiler during sanitizer build."
    FORCE)
set(CMAKE_EXE_LINKER_FLAGS_SANITIZER
    "" CACHE STRING
    "Flags used for linking binaries during sanitizer build."
    FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_SANITIZER
    "" CACHE STRING
    "Flags used by the shared libraries linker during sanitizer build."
    FORCE)

mark_as_advanced(
    CMAKE_C_FLAGS_SANITIZER
    CMAKE_EXE_LINKER_FLAGS_SANITIZER
    CMAKE_SHARED_LINKER_FLAGS_SANITIZER)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel Sanitizer)

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
    "Build type, options are: Debug Release RelWithDebInfo MinSizeRel Sanitizer."
    FORCE)

include_directories(include)
aux_source_directory(src SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

pkg_check_modules(JSONC REQUIRED json-c)

target_link_libraries(
  ${PROJECT_NAME} ${JSONC_LINK_LIBRARIES})
include_directories(${JSONC_INCLUDE_DIRS})
