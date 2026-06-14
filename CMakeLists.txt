cmake_minimum_required(VERSION 3.15)
project(OrderSimulator)

# Включаем стандарт C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Crow CONFIG REQUIRED)
find_package(Boost REQUIRED)

add_executable(order_api main.cpp)

target_include_directories(order_api PRIVATE "C:/vcpkg/installed/x64-windows/include")


target_link_libraries(order_api PRIVATE 
    Crow::Crow 
    Boost::boost 
    "C:/vcpkg/installed/x64-windows/lib/pqxx.lib"
    "C:/vcpkg/installed/x64-windows/lib/libpq.lib"
)

find_package(Threads REQUIRED)
target_link_libraries(order_api PRIVATE Threads::Threads)