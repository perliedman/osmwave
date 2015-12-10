cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOSMIUM_WITH_SPARSEHASH")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

project("osmwave")

include_directories(src)

add_executable(osmwave src/ObjWriter.cxx src/cli.cxx src/elevation.cxx src/osmwave.cxx)
add_executable(terrainobj src/terrain.cxx src/elevation.cxx src/ObjWriter.cxx src/Delaunay.cpp)
target_link_libraries(osmwave bz2 z expat pthread proj boost_regex boost_program_options)
target_link_libraries(terrainobj proj boost_program_options)
