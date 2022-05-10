
set(CC_EXTERNAL_LIBS)
set(CC_EXTERNAL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/sources)
set(CC_EXTERNAL_PRIVATE_INCLUDES)
set(CC_EXTERNAL_PRIVATE_DEFINITIONS)

list(APPEND CC_EXTERNAL_PRIVATE_DEFINITIONS
    TBB_USE_EXCEPTIONS=0 # no rtti for now
)
MESSAGE(STATUS "platform path: ${platform_spec_path}")
include(${CMAKE_CURRENT_LIST_DIR}/cmake/CocosExternalConfig.cmake)

if(WINDOWS)
    include(${CMAKE_CURRENT_LIST_DIR}/win64/CMakeLists.txt)
elseif(APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/mac/CMakeLists.txt)
elseif(LINUX)
    include(${CMAKE_CURRENT_LIST_DIR}/linux/CMakeLists.txt)
elseif(ANDROID)
    include(${CMAKE_CURRENT_LIST_DIR}/android/CMakeLists.txt)
endif()
