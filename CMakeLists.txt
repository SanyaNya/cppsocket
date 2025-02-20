cmake_minimum_required(VERSION 3.31)

project(cppsocket VERSION 1.4.0)

#Require C++23 without compiler extensions
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_EXAMPLES "Build examples" OFF)

file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.5/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:SanyaNya/hpp@1.6.1")
CPMAddPackage("gh:SanyaNya/ehl@1.1.1")
CPMAddPackage("gh:SanyaNya/system_errc@0.6.0")
CPMAddPackage("gh:SanyaNya/strict_enum@1.7.0")
CPMAddPackage("gh:SanyaNya/aggr_refl@1.2.1")

add_library(cppsocket INTERFACE)
target_link_libraries(cppsocket INTERFACE hpp ehl system_errc strict_enum aggr_refl)
target_include_directories(cppsocket INTERFACE ${cppsocket_SOURCE_DIR}/include)

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
