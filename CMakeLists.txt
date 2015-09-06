cmake_minimum_required(VERSION 2.8.11)

project(convutf8)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)

add_executable(convutf8 main.cpp)
target_link_libraries(convutf8 Qt5::Core)
set_property(TARGET convutf8 PROPERTY CXX_STANDARD 11)
