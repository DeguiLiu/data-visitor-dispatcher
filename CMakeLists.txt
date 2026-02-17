cmake_minimum_required(VERSION 3.14)
project(data_visitor_dispatcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- mccc-bus dependency (auto-download via FetchContent) ---
include(FetchContent)

set(MCCC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MCCC_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(mccc-bus
    GIT_REPOSITORY https://gitee.com/liudegui/mccc-bus.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(mccc-bus)

# --- Examples ---
add_subdirectory(examples)
