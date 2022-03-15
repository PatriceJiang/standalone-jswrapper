
set(USE_SE_V8 ON)
set(USE_V8_DEBUGGER ON)


include(${CMAKE_CURRENT_LIST_DIR}/cmake/predefine.cmake)
if(WINDOWS)
   add_definitions(-DCC_STATIC=1)
endif()

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_CURRENT_LIST_DIR}/external/CMakeLists.txt)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/base ccbase)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/fileutils ccfileutils)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/jswrapper ccjswrapper)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/math ccmath)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bindings ccbindings)