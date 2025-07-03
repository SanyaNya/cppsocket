include(cflags.cmake)

add_executable(server server.cpp)
target_link_libraries(server cppsocket)

add_executable(client client.cpp)
target_link_libraries(client cppsocket)

#add_executable(serverc server.c)

add_executable(server_udp_variant server_udp_variant.cpp)
target_link_libraries(server_udp_variant cppsocket)

add_executable(client_udp_variant client_udp_variant.cpp)
target_link_libraries(client_udp_variant cppsocket)
