cmake_minimum_required(VERSION 3.31)

project(cppsocket)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#Require C++23 without compiler extensions
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_EXAMPLES "Build examples" OFF)

include(Fetch)

fetch_project(hpp https://github.com/SanyaNya/hpp v1.2.0)
fetch_project(ehl https://github.com/SanyaNya/ehl v1.1.1)
fetch_project(system_errc https://github.com/SanyaNya/system_errc v0.5.1)
fetch_project(strict_enum https://github.com/SanyaNya/strict_enum v1.5.0)
fetch_project(aggr_refl https://github.com/SanyaNya/aggr_refl v1.2.0)

add_library(cppsocket INTERFACE)
target_link_libraries(cppsocket INTERFACE hpp ehl system_errc strict_enum aggr_refl)
target_include_directories(cppsocket INTERFACE ${cppsocket_SOURCE_DIR}/include)

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
