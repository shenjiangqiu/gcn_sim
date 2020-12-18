project(gcn_sim)
add_subdirectory(deps/fmt)
add_subdirectory(deps/ramulator)
set(CMAKE_CXX_STANDARD 17)
file(GLOB lib_srcs src/*.cc)
list(FILTER lib_srcs EXCLUDE REGEX  ".*main.cc")

add_library(gcn_sim_lib ${lib_srcs})
target_include_directories(gcn_sim_lib  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(gcn_sim_lib PUBLIC ramulator)

add_executable(gcn_sim ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc)

target_link_libraries(gcn_sim PUBLIC gcn_sim_lib fmt)


aux_source_directory(test TEST_SOURCE)
add_executable(gcn_sim_test ${TEST_SOURCE})
target_include_directories(gcn_sim_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/test)
target_link_libraries(gcn_sim_test PUBLIC gcn_sim_lib fmt)
configure_file(test/test.graph test.graph COPYONLY)
