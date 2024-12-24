include(cflags.cmake)

add_executable(server server.cpp)
target_link_libraries(server cppsocket)

add_executable(client client.cpp)
target_link_libraries(client cppsocket)

#add_executable(serverc server.c)
