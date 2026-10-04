cmake_minimum_required(VERSION 4.3)
project(isa C)

set(CMAKE_C_STANDARD 11)

add_library(isa STATIC isa.c isa.h)

target_include_directories(isa PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})