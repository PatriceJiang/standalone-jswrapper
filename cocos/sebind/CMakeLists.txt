cmake_minimum_required(VERSION 3.8)

project(sebind CXX)

add_library(sebind
    class_v8.h
    class_v8.cpp
    internal/common.h
    internal/common.cpp
    sebind.h
)


target_link_libraries(sebind PUBLIC
    ccbindings
)

target_include_directories(sebind PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)