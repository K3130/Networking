cmake_minimum_required (VERSION 3.17)
project(Server VERSION 0.1 LANGUAGES CXX)
set(PROJECT_NAME "Networking")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)


set(PATH_ASIO "C:\\vcpkg\\installed\\x64-windows\\include")

add_executable (${PROJECT_NAME} "Networking.cpp" "Networking.h" "net_common.h" "olc_net.h" "net_tsqueue.h" "net_server.h" "net_message.h" "net_connection.h" "net_client.h")

target_include_directories(${PROJECT_NAME} PRIVATE ${PATH_ASIO})


# TODO: Добавьте тесты и целевые объекты, если это необходимо.
