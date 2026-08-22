cmake_minimum_required(VERSION 3.16)
project(network_optimization_3 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(USE_CBC "Build the CbcMipSolver backend (requires COIN-OR CBC dev libraries)" OFF)

include_directories(include)

set(CORE_SOURCES
    src/ClusteringProblem.cpp
    src/GreedyNearestHubSolver.cpp
    src/BranchAndBoundSolver.cpp
)

add_library(network_opt3_core ${CORE_SOURCES})
target_include_directories(network_opt3_core PUBLIC include)

if(USE_CBC)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(CBC REQUIRED cbc)
  target_compile_definitions(network_opt3_core PUBLIC NETOPT3_USE_CBC)
  target_include_directories(network_opt3_core PUBLIC ${CBC_INCLUDE_DIRS})
  target_link_libraries(network_opt3_core PUBLIC ${CBC_LIBRARIES})
endif()

add_executable(network_opt3_demo src/main.cpp)
target_link_libraries(network_opt3_demo PRIVATE network_opt3_core)

add_executable(network_opt3_tests
    tests/main.cpp
    tests/ClusteringTests.cpp
)
target_include_directories(network_opt3_tests PRIVATE tests)
target_link_libraries(network_opt3_tests PRIVATE network_opt3_core)

enable_testing()
add_test(NAME unit_tests COMMAND network_opt3_tests)
