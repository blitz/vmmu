add_library(vmmu STATIC src/pt_walk.cpp src/tlb_entry.cpp)

target_include_directories(
  vmmu PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
              $<INSTALL_INTERFACE:${INCLUDEDIR}>)
target_compile_features(vmmu PUBLIC cxx_std_17)

set_target_properties(
  vmmu PROPERTIES PUBLIC_HEADER "include/vmmu.hpp;include/vmmu_utilities.hpp")

configure_file("vmmu.pc.in" "vmmu.pc" @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vmmu.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(
  TARGETS vmmu
  EXPORT vmmu-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT vmmu-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/vmmu)
