cmake_minimum_required(VERSION 3.14)
project(data_visitor_dispatcher LANGUAGES CXX)

# --- mccc-bus dependency ---
set(MCCC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../mccc-bus"
    CACHE PATH "Path to mccc-bus source tree")

if(NOT EXISTS "${MCCC_DIR}/CMakeLists.txt")
  message(FATAL_ERROR
    "mccc-bus not found at ${MCCC_DIR}\n"
    "Set -DMCCC_DIR=/path/to/mccc-bus or clone it next to this project.")
endif()

set(MCCC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MCCC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${MCCC_DIR} ${CMAKE_BINARY_DIR}/mccc-bus EXCLUDE_FROM_ALL)

# --- Demo: Component-based (dynamic subscribe) ---
add_executable(mccc_demo mccc_demo.cpp)
target_link_libraries(mccc_demo PRIVATE mccc_extras pthread)

# --- Demo: StaticComponent (zero-overhead CRTP) ---
add_executable(mccc_static_demo mccc_static_demo.cpp)
target_link_libraries(mccc_static_demo PRIVATE mccc_extras pthread)
