find_package(Catch2 REQUIRED)

add_executable(tests main.cpp test_memory.cpp test_paging_state.cpp
                     test_pt_walk.cpp test_tlb_attr.cpp test_tlb_entry.cpp)

target_link_libraries(tests PRIVATE Catch2::Catch2)
target_link_libraries(tests PRIVATE vmmu)

include(Catch)
catch_discover_tests(tests)
