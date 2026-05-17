cmake_minimum_required(VERSION 3.20)

if(APPLE AND NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER)
  foreach(DS_RT_LLVM_PREFIX IN ITEMS /opt/homebrew/opt/llvm /usr/local/opt/llvm)
    if(EXISTS "${DS_RT_LLVM_PREFIX}/bin/clang" AND EXISTS "${DS_RT_LLVM_PREFIX}/bin/clang++")
      set(CMAKE_C_COMPILER "${DS_RT_LLVM_PREFIX}/bin/clang" CACHE FILEPATH "C compiler" FORCE)
      set(CMAKE_CXX_COMPILER "${DS_RT_LLVM_PREFIX}/bin/clang++" CACHE FILEPATH "C++ compiler" FORCE)
      message(STATUS "Using LLVM toolchain from ${DS_RT_LLVM_PREFIX}")
      break()
    endif()
  endforeach()
endif()

project(ds_rt LANGUAGES C CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(ds_rt_warnings INTERFACE)
target_compile_options(ds_rt_warnings INTERFACE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wshadow
    -Wconversion
    -Wfloat-conversion
    -Wsign-conversion
    -Wcast-qual
    -Wcast-align
    -Wformat=2
    -Wundef
    -Wnull-dereference
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wold-style-cast
    -Wzero-as-null-pointer-constant
    -Wdouble-promotion
    -Wimplicit-fallthrough
    -Wreturn-type>
  $<$<CXX_COMPILER_ID:Clang,AppleClang>:
    -Wconditional-uninitialized
    -Wextra-semi
    -Wimplicit-int-conversion
    -Wimplicit-int-float-conversion
    -Wnewline-eof
    -Wshorten-64-to-32
    -Wswitch-enum
    -Wunreachable-code>
  $<$<CXX_COMPILER_ID:GNU>:
    -Wduplicated-branches
    -Wduplicated-cond
    -Wlogical-op
    -Wsign-compare
    -Wuseless-cast>
)

add_executable(ds_rt
  src/main.cpp
)
target_include_directories(ds_rt PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(ds_rt PRIVATE ds_rt_warnings)
