cmake_minimum_required(VERSION 3.10)
project(mini_os_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(os_simulator src/main.cpp)
# later: add src/process.cpp, src/scheduler.cpp, etc.
