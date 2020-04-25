cmake_minimum_required(VERSION 3.15)

project(
  vmmu
  VERSION 0.9.0
  DESCRIPTION "An implementation of the x86 MMU"
  HOMEPAGE_URL https://github.com/blitz/vmmu
  LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(CTest)

set(DEFAULT_BUILD_TYPE "Debug")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
      STRING "Choose the type of build." FORCE)

  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

option(ASAN "Enable the address sanitizer" OFF)

if(ASAN)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif()

option(BUILD_COVERAGE "Build with code coverage instrumentation" OFF)

if(BUILD_COVERAGE)
  include(CodeCoverage)
  append_coverage_compiler_flags()

  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/coverage
          DESTINATION ${CMAKE_INSTALL_DOCDIR}/vmmu/)

endif()

add_subdirectory(libvmmu)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
