cmake_minimum_required(VERSION 3.10)
project(lab5_7_variant31)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Вместо find_package(ZeroMQ REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

include_directories(${ZMQ_INCLUDE_DIRS})
link_directories(${ZMQ_LIBRARY_DIRS})

add_library(lab5_7_lib lib.cpp)

add_executable(control control.cpp)
add_executable(computing computing.cpp)

target_include_directories(lab5_7_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(lab5_7_lib PRIVATE ${ZMQ_LIBRARIES})
target_link_libraries(control PRIVATE lab5_7_lib ${ZMQ_LIBRARIES})
target_link_libraries(computing PRIVATE lab5_7_lib ${ZMQ_LIBRARIES})
