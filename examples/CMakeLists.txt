include(cflags.cmake)

add_executable(init init.cpp)
target_link_libraries(init cppsocket)

add_executable(initc init.c)
