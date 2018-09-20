PROJECT(ResolutionGraph)

cmake_minimum_required (VERSION 2.6)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
SET(CMAKE_C_FLAGS_RELEASE "-O3 -march=native")

FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_executable(ResolutionGraph main.cpp literal.cpp clause.cpp solver_shadow.cpp resolution_graph.cpp)
target_link_libraries( ResolutionGraph ${Boost_LIBRARIES} )

