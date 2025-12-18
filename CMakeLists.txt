cmake_minimum_required(VERSION 3.11)

project(hpg C)
include(FetchContent)

option(CXX "enable C++ compilation" ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -march=native")

if(CXX)
  enable_language(CXX)
endif()

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(MSVC AND MT)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
endif()

find_package(GUROBI)

if(GUROBI_FOUND)
  message(STATUS "Gurobi found")
  include_directories(${GUROBI_INCLUDE_DIRS}) 
  include_directories(${CMAKE_SOURCE_DIR}/include)

  add_executable(compactSolver.exe src/Instance.cpp src/Solution.cpp src/CompactModel.cpp compactSolver.cpp)
  target_link_libraries(compactSolver.exe optimized ${GUROBI_CXX_LIBRARY}
                          debug ${GUROBI_CXX_DEBUG_LIBRARY})
  target_link_libraries(compactSolver.exe ${GUROBI_LIBRARY})
  target_compile_definitions(compactSolver.exe PRIVATE USE_GUROBI)

  add_executable(colGenSolver.exe src/Instance.cpp src/Solution.cpp src/Heuristics.cpp src/Column.cpp src/ColGenModel.cpp src/DivingHeuristic.cpp colGenSolver.cpp)
  target_link_libraries(colGenSolver.exe optimized ${GUROBI_CXX_LIBRARY}
                          debug ${GUROBI_CXX_DEBUG_LIBRARY})
  target_link_libraries(colGenSolver.exe ${GUROBI_LIBRARY})
  target_compile_definitions(colGenSolver.exe PRIVATE USE_GUROBI)

  add_executable(benchmark.exe src/Instance.cpp src/Solution.cpp src/Heuristics.cpp src/Column.cpp src/ColGenModel.cpp src/CompactModel.cpp benchmark.cpp)
  target_link_libraries(benchmark.exe optimized ${GUROBI_CXX_LIBRARY}
                          debug ${GUROBI_CXX_DEBUG_LIBRARY})
  target_link_libraries(benchmark.exe ${GUROBI_LIBRARY})
  target_compile_definitions(benchmark.exe PRIVATE USE_GUROBI)
else()
  message(STATUS "Gurobi not found")
endif()