cmake_minimum_required(VERSION 3.10)
project(mini_os_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project-wide include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# ===============================
# Scheduler library
# ===============================
add_library(scheduler STATIC
    src/fcfs_scheduler.cpp
    src/rr_scheduler.cpp
    src/sjf_scheduler.cpp
    src/priority_scheduler.cpp
)
target_include_directories(scheduler PUBLIC ${PROJECT_SOURCE_DIR}/include)

# ===============================
# Memory management library
# ===============================
add_library(memory STATIC
    src/memory_manager.cpp
    src/paging.cpp
)
target_include_directories(memory PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(memory PUBLIC scheduler)

# ===============================
# Filesystem library
# ===============================
add_library(filesys STATIC
    src/filesys.cpp
)
target_include_directories(filesys PUBLIC ${PROJECT_SOURCE_DIR}/include)

# ===============================
# Runner library (simulation orchestration, no main)
# ===============================
add_library(runner_core STATIC
    src/runner.cpp
)
target_include_directories(runner_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(runner_core PUBLIC scheduler memory filesys)

# ===============================
# Executables (demos)
# Each one has its own main()
# ===============================

# Main OS simulator
add_executable(os_simulator src/main.cpp)
target_link_libraries(os_simulator PRIVATE scheduler memory filesys runner_core)

# Memory demo
add_executable(memory_demo src/memory_demo.cpp)
target_link_libraries(memory_demo PRIVATE memory scheduler filesys)

# Paging demo
add_executable(paging_demo src/paging_demo.cpp)
target_link_libraries(paging_demo PRIVATE memory)

# Filesystem demo
add_executable(filesys_demo src/filesys_demo.cpp)
target_link_libraries(filesys_demo PRIVATE filesys)

# Runner demo
add_executable(runner_demo src/runner_demo.cpp)
target_link_libraries(runner_demo PRIVATE runner_core scheduler memory filesys)
