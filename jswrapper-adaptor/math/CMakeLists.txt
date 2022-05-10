cmake_minimum_required(VERSION 3.8)

project(ccmath CXX)

enable_language(ASM C)

set(module_name ccmath)

set(math_source
    include/math/Geometry.h
    include/math/Mat3.h
    include/math/Mat4.h
    include/math/MathBase.h
    include/math/Math.h
    include/math/MathUtil.h
    include/math/Quaternion.h
    include/math/Vec2.h
    include/math/Vec3.h
    include/math/Vec4.h
    include/math/Vertex.h 
    include/math/Vec2.inl
    include/math/Vec3.inl
    include/math/Vec4.inl
    include/math/Mat4.inl
    include/math/Quaternion.inl
    src/Geometry.cpp
    src/Mat3.cpp
    src/Mat4.cpp
    src/Math.cpp
    src/MathUtil.cpp
    src/MathUtil.inl
    src/MathUtilNeon.inl
    src/MathUtilNeon64.inl
    src/MathUtilSSE.inl
    src/Quaternion.cpp
    src/Vec2.cpp
    src/Vec3.cpp
    src/Vec4.cpp
    src/Vertex.cpp
)

file(GLOB export_headers "${CMAKE_CURRENT_LIST_DIR}/include/math/*.h")

add_library(${module_name} ${math_source})

set_property(TARGET ${module_name} 
     PROPERTY CXX_STANDARD 14)

set_target_properties(${module_name} PROPERTIES PUBLIC_HEADER "${export_headers}")


if(ANDROID)
    target_link_libraries(${module_name} PUBLIC 
        android_platform
    )
endif()

target_include_directories(${module_name} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(${module_name} PUBLIC
    ccbase
)


# INSTALL(TARGETS ${module_name} 
#         PUBLIC_HEADER DESTINATION include/math
# )