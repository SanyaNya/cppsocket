cmake_minimum_required(VERSION 3.20) #c++23

project(cppsocket)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#Require C++23 without compiler extensions
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_EXAMPLES "Build examples" OFF)

include(Fetch)

fetch_project(ehl https://github.com/SanyaNya/ehl v1.1.1)
fetch_project(strict_enum https://github.com/SanyaNya/strict_enum v1.3.2)

add_library(cppsocket INTERFACE)
target_link_libraries(cppsocket INTERFACE ehl strict_enum)
target_include_directories(cppsocket INTERFACE ${cppsocket_SOURCE_DIR}/include)

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
