# Component version: dynamic subscribe/unsubscribe at runtime
add_executable(component_demo component_demo.cpp)
target_link_libraries(component_demo PRIVATE mccc_extras pthread)

# StaticComponent version: zero-overhead compile-time dispatch
add_executable(static_component_demo static_component_demo.cpp)
target_link_libraries(static_component_demo PRIVATE mccc_extras pthread)
