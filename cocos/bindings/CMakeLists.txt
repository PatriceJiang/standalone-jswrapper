cmake_minimum_required(VERSION 3.8)

project(bindings CXX)

set(module_name ccbindings)

add_library(${module_name}
    conversions/jsb_conversions.h
    conversions/jsb_conversions_spec.h
    conversions/jsb_conversions_spec.cpp
    conversions/jsb_classtype.h
    conversions/jsb_classtype.cpp
    conversions/jsb_global.cpp
    conversions/jsb_global.h
)

target_include_directories(${module_name} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${module_name} PUBLIC 
    ccbase
    ccjswrapper
    ccmath
    ccfileutils
)

set_target_properties(${module_name} PROPERTIES
    CXX_STANDARD 17
)

target_compile_definitions(${module_name} PUBLIC
    USE_CXX_17=1
)
