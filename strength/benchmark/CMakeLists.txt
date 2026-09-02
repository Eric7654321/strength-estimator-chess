add_executable(benchmark_forward benchmark_forward.cpp)
target_include_directories(benchmark_forward PRIVATE ${CMAKE_SOURCE_DIR}/strength/misc)
target_link_libraries(
    benchmark_forward
    network
    utils
    actor
    config
    console
    environment
)