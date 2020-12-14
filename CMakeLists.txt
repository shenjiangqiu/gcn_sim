project(gcn_sim)
add_subdirectory(deps/fmt)
aux_source_directory(src SOURCE)

add_executable(gcn_sim ${SOURCE})
target_include_directories(gcn_sim  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(gcn_sim PUBLIC fmt)
