cmake_minimum_required(VERSION 3.8)

project(bindings CXX)

add_library(bindings
    conversions/jsb_conversions.h
    conversions/jsb_conversions_spec.h
    conversions/jsb_conversions_spec.cpp
    conversions/jsb_classtype.h
    conversions/jsb_classtype.cpp
    conversions/jsb_global.cpp
    conversions/jsb_global.h
)

target_include_directories(bindings PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(bindings PUBLIC 
    ccbase
    jswrapper
    ccmath
    ccfileutils
)

set_target_properties(bindings PROPERTIES
    CXX_STANDARD 17
)

target_compile_definitions(bindings PUBLIC
    USE_CXX_17=1
)
