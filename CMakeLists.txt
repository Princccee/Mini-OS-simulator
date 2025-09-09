cmake_minimum_required(VERSION 3.10)
project(mini_os_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(os_simulator
    src/main.cpp
    src/fcfs_scheduler.cpp
    src/rr_scheduler.cpp
)
