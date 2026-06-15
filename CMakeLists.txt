cmake_minimum_required(VERSION 3.15)
project(OrderSimulator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ищем пакеты
find_package(Boost REQUIRED COMPONENTS system)
find_package(postgresql REQUIRED)
find_package(Crow REQUIRED)
find_package(RdKafka CONFIG REQUIRED) # <-- Добавили Кафку

find_package(pqxx CONFIG)
if(NOT pqxx_FOUND)
    find_package(libpqxx REQUIRED)
endif()

add_executable(OrderSimulator main.cpp)

if(TARGET pqxx::pqxx)
    set(PQXX_LIBRARY pqxx::pqxx)
else()
    set(PQXX_LIBRARY libpqxx::pqxx)
endif()

# Линкуем всё вместе
target_link_libraries(OrderSimulator PRIVATE 
    Crow::Crow
    ${PQXX_LIBRARY}
    Boost::system
    RdKafka::rdkafka++ # <-- Добавили C++ обертку Кафки
)