cmake_minimum_required(VERSION 3.8)

project(setpl CXX)

add_library(setpl
    class.h
    "function.h"
    module.h
    signature.h
    class.cpp
)


target_link_libraries(setpl PUBLIC
    ccbindings
)

target_include_directories(setpl PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)