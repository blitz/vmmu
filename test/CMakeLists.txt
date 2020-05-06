find_package(Catch2 REQUIRED)

add_executable(tests main.cpp test_memory.cpp test_pt_walk.cpp
                     test_tlb_attr.cpp test_tlb_entry.cpp)

target_link_libraries(tests PRIVATE Catch2::Catch2)
target_link_libraries(tests PRIVATE vmmu)

if(BUILD_COVERAGE)
  setup_target_for_coverage_gcovr_html(NAME coverage-html EXECUTABLE tests
                                       EXCLUDE "test/*")

  setup_target_for_coverage_gcovr_xml(NAME coverage-xml EXECUTABLE tests
                                      EXCLUDE "test/*")

  add_custom_target(build_coverage ALL)
  add_dependencies(build_coverage coverage-xml)
endif()

include(Catch)
catch_discover_tests(tests)
