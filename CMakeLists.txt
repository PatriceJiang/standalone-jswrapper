cmake_minimum_required(VERSION 3.8)

project(jswrapper CXX)


include(${CMAKE_CURRENT_LIST_DIR}/cocos/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/jswrapper-adaptor/CMakeLists.txt)


add_executable(demo
   demo/Tank.h
   demo/Tank.cpp
   
   # genbindings 生成的代码
   demo/auto/jsb_demo_auto.cpp
   demo/auto/jsb_demo_auto.h
   
   demo/test.cpp
)

target_link_libraries(demo ccbindings)

set(target_name demo)

if(MSVC)
   file(GLOB WINDOWS_DLLS cocos/external/win64/libs/*.dll)
   foreach(item ${WINDOWS_DLLS})
        get_filename_component(filename ${item} NAME)
        get_filename_component(abs ${item} ABSOLUTE)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${abs} $<TARGET_FILE_DIR:${target_name}>/${filename}
        )
    endforeach()
    target_link_options(${target_name} PRIVATE /SUBSYSTEM:CONSOLE)
endif()

add_custom_command(TARGET ${target_name} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/demo/assets/hello.js $<TARGET_FILE_DIR:${target_name}>/hello.js
)