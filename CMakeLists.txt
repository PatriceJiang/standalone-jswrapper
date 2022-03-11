cmake_minimum_required(VERSION 3.8)

project(jswrapper CXX)




set(USE_SE_V8 ON)
set(USE_V8_DEBUGGER ON)


include(cmake/predefine.cmake)
if(WINDOWS)
   add_definitions(-DCC_STATIC=1)
endif()

set(CMAKE_CXX_STANDARD 17)

include(external/CMakeLists.txt)

add_subdirectory(base ccbase)
add_subdirectory(fileutils ccfileutils)
add_subdirectory(jswrapper jswrapper)
add_subdirectory(math ccmath)
add_subdirectory(bindings bindings)

add_executable(demo
   demo/Tank.h
   demo/Tank.cpp
   demo/auto/jsb_demo_auto.cpp
   demo/auto/jsb_demo_auto.h

   demo/seinit/jsb_global_init.h
   demo/seinit/jsb_global_init.cpp

   demo/test.cpp
)

target_link_libraries(demo bindings)

set(target_name demo)

if(MSVC)
   file(GLOB WINDOWS_DLLS external/win64/libs/*.dll)
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