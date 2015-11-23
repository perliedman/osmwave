cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOSMIUM_WITH_SPARSEHASH")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

project("osmwave")

include_directories(src)
file(GLOB SOURCES "src/*.cxx")

add_executable(osmwave ${SOURCES})
target_link_libraries(osmwave bz2 z expat pthread proj boost_regex boost_program_options)
