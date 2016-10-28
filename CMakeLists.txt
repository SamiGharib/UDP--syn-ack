cmake_minimum_required(VERSION 3.2)
project(receiver)


set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall  -Werror  -Wshadow  -Wextra  -O2 -D_FORTIFY_SOURCE=2  -fstack-protector-all  -g -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer -lz")

add_executable(receiver src/receiver/receiver.c)

add_library(packet_interface STATIC src/shared/packet_interface.c)
add_library(real_adress STATIC  src/shared/real_address.c)
add_library(wait_for_client STATIC src/shared/wait_for_client.c)
add_library(create_socket STATIC src/shared/create_socket.c)
add_library(gbnHelper STATIC src/receiver/gbnHelper.c)

target_link_libraries(packet_interface z)

target_link_libraries(gbnHelper packet_interface)

target_link_libraries(receiver packet_interface)
target_link_libraries(receiver real_adress)
target_link_libraries(receiver wait_for_client)
target_link_libraries(receiver gbnHelper)
target_link_libraries(receiver create_socket)


project(sender)

add_executable(sender src/sender/sender.c)

add_library(sender_help STATIC src/sender/sender_help.c)

target_link_libraries(sender sender_help)
target_link_libraries(sender packet_interface)
target_link_libraries(sender real_adress)
target_link_libraries(sender create_socket)
target_link_libraries(sender sender_help)

#enable_testing ()

#add_executable (test_lib tests/tests.c)
#target_link_libraries(test_lib sender_help)
#target_link_libraries(test_lib packet_interface)
#target_link_libraries(test_lib real_adress)
#target_link_libraries(test_lib create_socket)

#set_target_properties (test_lib PROPERTIES LINK_FLAGS -Wl,-lcunit)

#add_test (test_lib test_lib)
