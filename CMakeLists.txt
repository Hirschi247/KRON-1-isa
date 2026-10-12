cmake_minimum_required(VERSION 3.16)
project(KRON-1-isa C)

set(CMAKE_C_STANDARD 11)

add_library(KRON-1-isa STATIC isa.c isa.h)

target_include_directories(KRON-1-isa PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Tests nur bauen, wenn das Repo eigenständig gebaut wird, nicht über add_subdirectory
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
    add_executable(test_isa test/test_isa.c)
    target_link_libraries(test_isa PRIVATE KRON-1-isa)
    add_test(NAME test_isa COMMAND test_isa)
endif ()
