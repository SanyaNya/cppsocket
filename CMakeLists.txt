cmake_minimum_required(VERSION 3.20) #c++23

project(cppsocket)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#Require C++23 without compiler extensions
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(Fetch)

fetch_project(ehl https://github.com/SanyaNya/ehl v1.1.0)

add_library(cppsocket INTERFACE)
target_link_libraries(cppsocket INTERFACE ehl)
target_include_directories(cppsocket INTERFACE ${cppsocket_SOURCE_DIR})
