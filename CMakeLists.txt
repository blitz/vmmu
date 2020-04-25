cmake_minimum_required(VERSION 3.15)

project(
  vmmu
  VERSION 0.9.0
  DESCRIPTION "An implementation of the x86 MMU"
  HOMEPAGE_URL https://github.com/blitz/vmmu
  LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

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
