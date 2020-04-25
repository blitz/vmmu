cmake_minimum_required(VERSION 3.15)

project(
  vmmu
  VERSION 0.9.0
  DESCRIPTION "An implementation of the x86 MMU"
  HOMEPAGE_URL https://github.com/blitz/vmmu
  LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

add_subdirectory(libvmmu)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
